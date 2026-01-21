#ifndef DARWINCORE_NETWORK_PROTOCOL_H
#define DARWINCORE_NETWORK_PROTOCOL_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <functional>

/**
 * @file protocol.h
 * @brief 网络协议编解码器 - 提供消息分片、流式传输和 CRC32 校验功能
 *
 * @par 功能概述
 * - 消息自动分片：支持将大消息自动分割成多个 Frame 发送
 * - 粘包处理：Decoder 自动处理 TCP 粘包问题
 * - 流式传输：支持大数据的流式传输（StreamStart/Chunk/End）
 * - CRC32 校验：可选的数据完整性校验
 * - 超时清理：自动清理未完成的消息碎片
 *
 * @par 使用示例
 *
 * @par 发送消息（编码）:
 * @code
 * // 使用协议编码
 * auto frames = Encoder::EncodeMessage(
 *     next_message_id_++,
 *     reinterpret_cast<const uint8_t*>(input.c_str()),
 *     input.size());
 *
 * auto serialized = Encoder::SerializeFrames(frames);
 *
 * // 发送编码后的数据包
 * for (const auto& packet : serialized) {
 *   if (!client_.SendData(packet.data(), packet.size())) {
 *     std::cerr << "发送失败！" << std::endl;
 *     break;
 *   }
 * }
 * @endcode
 *
 * @par 接收消息（解码）:
 * @code
 * // 使用协议解码器处理数据（处理粘包）
 * try {
 *   // 喂入数据
 *   decoder_.Feed(data.data(), data.size());
 *
 *   // 获取完整的消息
 *   MessageComplete msg;
 *   while (decoder_.GetMessage(msg)) {
 *     std::string text(msg.data.begin(), msg.data.end());
 *     std::cout << "[服务器响应] " << text;
 *     std::cout.flush();
 *   }
 *
 *   // 处理流事件
 *   StreamEvent event;
 *   while (decoder_.GetStreamEvent(event)) {
 *     std::cout << "[流事件] 流ID: " << event.stream_id
 *               << " 类型: " << static_cast<int>(event.type) << std::endl;
 *   }
 * } catch (const ProtocolError& e) {
 *   std::cerr << "[协议错误] " << e.what() << std::endl;
 * }
 * @endcode
 */

namespace darwincore
{
    namespace network
    {

        namespace proto
        {
            // ========== 协议常量定义 ==========

            /** @brief 魔数第1字节（用于帧同步） */
            constexpr uint8_t MAGIC1 = 0x5A;
            /** @brief 魔数第2字节（用于帧同步） */
            constexpr uint8_t MAGIC2 = 0x5C;
            /** @brief 协议版本号 */
            constexpr uint8_t VERSION = 0x01;

            /** @brief 单个 Frame 的最大 Payload 大小（256KB） */
            constexpr uint32_t MAX_FRAME_PAYLOAD = 256 * 1024;
            /** @brief 单条消息允许的最大分片数 */
            constexpr uint16_t MAX_MESSAGE_SLICES = 65535;
            /** @brief 消息组装超时时间（30秒） */
            constexpr uint32_t DEFAULT_MESSAGE_TIMEOUT_MS = 30000;

            // ========== 帧标志位 ==========

            /** @brief CRC32 校验标志（表示 payload 包含 CRC32 校验） */
            constexpr uint16_t FLAG_CRC32 = 0x0001;

            // ========== 帧类型枚举 ==========

            /** @brief 帧类型枚举 */
            enum class FrameType : uint8_t
            {
                /** @brief 普通消息帧（支持分片） */
                Message = 0x01,
                /** @brief 流式传输开始帧 */
                StreamStart = 0x02,
                /** @brief 流式传输数据块帧 */
                StreamChunk = 0x03,
                /** @brief 流式传输结束帧 */
                StreamEnd = 0x04,
            };

            // ========== 帧头结构（16 字节，紧凑布局） ==========

#pragma pack(push, 1)
            /** @brief 帧头结构（16 字节，网络字节序） */
            struct FrameHeader
            {
                uint8_t magic1;          /**< 魔数第1字节 (0x5A) */
                uint8_t magic2;          /**< 魔数第2字节 (0x5C) */
                uint8_t version;         /**< 协议版本号 */
                uint8_t type;            /**< 帧类型（FrameType 枚举值） */
                uint16_t flags;          /**< 标志位（如 FLAG_CRC32） */
                uint32_t payload_len;    /**< Payload 长度（字节数） */
                uint32_t reserved;       /**< 保留字段（未来扩展用） */
                uint16_t reserved2;      /**< 额外的保留字段，使头部为 16 字节 */
            };
#pragma pack(pop)

            static_assert(sizeof(FrameHeader) == 16);

            // ========== 消息帧专用结构 ==========

#pragma pack(push, 1)
            /** @brief 消息帧头部（用于 Message 类型帧） */
            struct MessageHeader
            {
                uint64_t message_id;     /**< 消息唯一标识符 */
                uint16_t total_slices;   /**< 消息总分片数 */
                uint16_t sequence;       /**< 当前分片序号（从 0 开始） */
            };
#pragma pack(pop)

            // ========== 流式传输专用结构 ==========

#pragma pack(push, 1)
            /** @brief 流开始帧的 Payload */
            struct StreamStartPayload
            {
                uint64_t stream_id;      /**< 流唯一标识符 */
                uint64_t total_size;     /**< 流的总大小（0 表示未知） */
            };
#pragma pack(pop)

#pragma pack(push, 1)
            /** @brief 流数据块帧的 Payload（后跟实际数据） */
            struct StreamChunkPayload
            {
                uint64_t stream_id;      /**< 流唯一标识符 */
                uint64_t offset;         /**< 数据在流中的偏移量 */
                // 后跟实际数据字节
            };
#pragma pack(pop)

#pragma pack(push, 1)
            /** @brief 流结束帧的 Payload */
            struct StreamEndPayload
            {
                uint64_t stream_id;      /**< 流唯一标识符 */
                uint32_t crc32;          /**< 整个流的 CRC32 校验值（可选） */
            };
#pragma pack(pop)

            // ========== Frame 基础结构 ==========

            /** @brief 帧结构（Header + Payload） */
            struct Frame
            {
                FrameHeader header;                /**< 帧头 */
                std::vector<uint8_t> payload;      /**< 载荷数据 */

                /**
                 * @brief 序列化为字节流（用于网络发送）
                 * @return 序列化后的字节数组
                 */
                std::vector<uint8_t> Serialize() const;

                /**
                 * @brief 从字节流反序列化（验证 magic 和 version）
                 * @param data 输入数据
                 * @param len 数据长度
                 * @param out 输出的 Frame 对象
                 * @return 成功返回 true，失败返回 false
                 */
                static bool Deserialize(const uint8_t *data, size_t len, Frame &out);
            };

            // ========== 协议异常类 ==========

            /** @brief 协议错误异常类 */
            class ProtocolError : public std::runtime_error
            {
            public:
                using std::runtime_error::runtime_error;
            };

            // ========== 解码器统计信息 ==========

            /** @brief 解码器统计信息 */
            struct DecoderStats
            {
                uint64_t frames_received = 0;      /**< 接收到的帧总数 */
                uint64_t messages_completed = 0;   /**< 完整接收的消息数 */
                uint64_t stream_events = 0;        /**< 流事件总数 */
                uint64_t bytes_received = 0;       /**< 接收到的字节总数 */
                uint64_t crc_errors = 0;           /**< CRC32 校验失败次数 */
                uint64_t timeout_cleanups = 0;     /**< 超时清理的消息数 */
                size_t pending_messages = 0;       /**< 当前正在组装的消息数 */
                size_t buffer_size = 0;            /**< 当前缓冲区大小 */
            };

        } // namespace proto

        namespace proto
        {
            // ========== 编码器类 ==========

            /**
             * @brief 协议编码器（静态工具类）
             *
             * 提供消息编码功能，支持：
             * - 自动分片：大消息自动分割成多个 Frame
             * - 流式传输：支持流式数据的编码
             * - CRC32 校验：可选的数据完整性保护
             */
            class Encoder
            {
            public:
                /**
                 * @brief 编码普通消息（自动分包）
                 * @param message_id 消息唯一标识符
                 * @param data 消息数据
                 * @param length 数据长度
                 * @param enable_crc 是否启用 CRC32 校验
                 * @return 编码后的帧数组
                 * @throw ProtocolError 消息过大时抛出异常
                 */
                static std::vector<Frame> EncodeMessage(
                    uint64_t message_id,
                    const uint8_t *data,
                    size_t length,
                    bool enable_crc = false);

                // ========== 流式传输编码 ==========

                /**
                 * @brief 编码流开始帧
                 * @param stream_id 流唯一标识符
                 * @param total_size 流的总大小（0 表示未知）
                 * @return 流开始帧
                 */
                static Frame EncodeStreamStart(
                    uint64_t stream_id,
                    uint64_t total_size);

                /**
                 * @brief 编码流数据块帧
                 * @param stream_id 流唯一标识符
                 * @param offset 数据在流中的偏移量
                 * @param data 数据
                 * @param length 数据长度
                 * @return 流数据块帧
                 * @throw ProtocolError 数据块过大时抛出异常
                 */
                static Frame EncodeStreamChunk(
                    uint64_t stream_id,
                    uint64_t offset,
                    const uint8_t *data,
                    size_t length);

                /**
                 * @brief 编码流结束帧
                 * @param stream_id 流唯一标识符
                 * @param crc32 整个流的 CRC32 校验值（0 表示不校验）
                 * @return 流结束帧
                 */
                static Frame EncodeStreamEnd(
                    uint64_t stream_id,
                    uint32_t crc32 = 0);

                // ========== 工具方法 ==========

                /**
                 * @brief 将 Frame 数组序列化为字节流数组
                 * @param frames 帧数组
                 * @return 序列化后的字节流数组（可直接用于网络发送）
                 */
                static std::vector<std::vector<uint8_t>> SerializeFrames(
                    const std::vector<Frame> &frames);

            private:
                /**
                 * @brief 创建一个帧的内部方法
                 * @param type 帧类型
                 * @param payload Payload 数据
                 * @param len Payload 长度
                 * @param crc 是否添加 CRC32 校验
                 * @return 构造好的帧
                 */
                static Frame MakeFrame(FrameType type, const void *payload, size_t len, bool crc = false);
            };

        } // namespace proto

        namespace proto
        {
            // ========== 解码器数据结构 ==========

            /** @brief 完整接收的消息 */
            struct MessageComplete
            {
                uint64_t message_id;           /**< 消息唯一标识符 */
                std::vector<uint8_t> data;     /**< 消息数据（已组装完整） */
            };

            /** @brief 流事件 */
            struct StreamEvent
            {
                FrameType type;                /**< 事件类型 */
                uint64_t stream_id;            /**< 流唯一标识符 */
                uint64_t offset = 0;           /**< 数据偏移量（仅 StreamChunk 有效） */
                uint64_t total_size = 0;       /**< 流的总大小（仅 StreamStart 有效） */
                uint32_t crc32 = 0;            /**< CRC32 校验值（仅 StreamEnd 有效） */
                std::vector<uint8_t> data;     /**< 数据块（仅 StreamChunk 有效） */
            };

            // ========== 解码器类 ==========

            /**
             * @brief 协议解码器
             *
             * 功能特性：
             * - 粘包处理：自动处理 TCP 粘包问题
             * - 消息重组：自动组装分片的消息
             * - 流式支持：支持流式数据的解码
             * - CRC32 校验：自动校验带 CRC 的帧
             * - 超时清理：自动清理未完成的消息
             *
             * @par 使用流程：
             * 1. 调用 Feed() 喂入网络数据
             * 2. 调用 GetMessage() 获取完整消息
             * 3. 调用 GetStreamEvent() 获取流事件
             */
            class Decoder
            {
            public:
                /**
                 * @brief 构造解码器
                 * @param message_timeout_ms 消息组装超时时间（毫秒）
                 */
                explicit Decoder(uint32_t message_timeout_ms = DEFAULT_MESSAGE_TIMEOUT_MS);

                /**
                 * @brief 喂入数据（支持多次调用，自动处理粘包）
                 * @param data 数据指针
                 * @param len 数据长度
                 * @throw ProtocolError 协议错误时抛出异常
                 */
                void Feed(const uint8_t *data, size_t len);

                /**
                 * @brief 获取完整的消息
                 * @param out 输出消息
                 * @return 有消息返回 true，无消息返回 false
                 */
                bool GetMessage(MessageComplete &out);

                /**
                 * @brief 获取流事件
                 * @param out 输出流事件
                 * @return 有事件返回 true，无事件返回 false
                 */
                bool GetStreamEvent(StreamEvent &out);

                /**
                 * @brief 获取统计信息
                 * @return 统计信息结构
                 */
                DecoderStats GetStats() const;

                /**
                 * @brief 清理超时的消息
                 * @return 清理的消息数量
                 */
                size_t CleanupTimeoutMessages();

                /**
                 * @brief 清空所有状态（重置解码器）
                 */
                void Reset();

            private:
                std::vector<uint8_t> buffer_;  /**< 接收缓冲区 */

                /** @brief 消息组装状态 */
                struct MessageAssembly
                {
                    uint16_t total = 0;                                    /**< 总分片数 */
                    std::vector<std::vector<uint8_t>> slices;             /**< 分片数据 */
                    uint16_t received = 0;                                /**< 已接收分片数 */
                    std::chrono::steady_clock::time_point first_seen;     /**< 首次见到时间 */
                };

                std::unordered_map<uint64_t, MessageAssembly> messages_;  /**< 正在组装的消息 */
                std::vector<MessageComplete> completed_messages_;         /**< 已完成的消息队列 */
                std::vector<StreamEvent> stream_events_;                  /**< 流事件队列 */
                uint32_t message_timeout_ms_;                             /**< 消息超时时间 */
                DecoderStats stats_;                                      /**< 统计信息 */

                /**
                 * @brief 尝试从缓冲区解码数据
                 * @throw ProtocolError 协议错误时抛出异常
                 */
                void TryDecode();
            };

            // ========== CRC32 工具函数 ==========

            /**
             * @brief 计算 CRC32 校验值（标准多项式：0xEDB88320）
             * @param data 数据指针
             * @param len 数据长度
             * @return CRC32 校验值
             */
            uint32_t CalculateCRC32(const uint8_t *data, size_t len);

        } // namespace proto

    } // namespace network
} // namespace darwincore

#endif
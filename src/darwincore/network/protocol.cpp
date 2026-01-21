#include <darwincore/network/protocol.h>
#include <algorithm>
#include <iostream>

namespace darwincore
{
    namespace network
    {

        namespace proto
        {

            // ========== Frame 序列化实现 ==========

            /**
             * @brief 将帧序列化为字节流
             * @return 序列化后的字节数组（Header + Payload）
             */
            std::vector<uint8_t> Frame::Serialize() const
            {
                std::vector<uint8_t> buffer(sizeof(FrameHeader) + payload.size());

                // 复制头部
                std::memcpy(buffer.data(), &header, sizeof(FrameHeader));

                // 复制 payload
                if (!payload.empty())
                {
                    std::memcpy(buffer.data() + sizeof(FrameHeader), payload.data(), payload.size());
                }

                return buffer;
            }

            /**
             * @brief 从字节流反序列化帧
             * @param data 输入数据
             * @param len 数据长度
             * @param out 输出的帧
             * @return 成功返回 true，失败返回 false
             */
            bool Frame::Deserialize(const uint8_t *data, size_t len, Frame &out)
            {
                if (len < sizeof(FrameHeader))
                    return false;

                // 复制头部
                std::memcpy(&out.header, data, sizeof(FrameHeader));

                // 验证魔数和版本
                if (out.header.magic1 != MAGIC1 || out.header.magic2 != MAGIC2)
                    return false;

                if (out.header.version != VERSION)
                    return false;

                // 检查长度
                size_t expected_size = sizeof(FrameHeader) + out.header.payload_len;
                if (len < expected_size)
                    return false;

                // 复制 payload
                if (out.header.payload_len > 0)
                {
                    out.payload.assign(data + sizeof(FrameHeader),
                                       data + expected_size);
                }
                else
                {
                    out.payload.clear();
                }

                return true;
            }

            // ========== Encoder 实现 ==========

            /**
             * @brief 创建一个帧的内部方法
             * @param type 帧类型
             * @param payload Payload 数据
             * @param len Payload 长度
             * @param crc 是否添加 CRC32 校验
             * @return 构造好的帧
             * @throw ProtocolError Payload 过大时抛出异常
             */
            Frame Encoder::MakeFrame(FrameType type, const void *payload, size_t len, bool crc)
            {
                if (len > MAX_FRAME_PAYLOAD)
                    throw ProtocolError("payload too large");

                Frame f{};
                f.header.magic1 = MAGIC1;
                f.header.magic2 = MAGIC2;
                f.header.version = VERSION;
                f.header.type = static_cast<uint8_t>(type);
                f.header.flags = crc ? FLAG_CRC32 : 0;
                f.header.payload_len = static_cast<uint32_t>(len);
                f.header.reserved = 0;
                f.header.reserved2 = 0;

                f.payload.resize(len);
                if (len && payload)
                    std::memcpy(f.payload.data(), payload, len);

                // 如果启用 CRC，在 payload 末尾追加 4 字节 CRC32
                if (crc)
                {
                    uint32_t crc_value = CalculateCRC32(f.payload.data(), f.payload.size());

                    // 扩展 payload 以容纳 CRC
                    size_t old_size = f.payload.size();
                    f.payload.resize(old_size + sizeof(uint32_t));
                    std::memcpy(f.payload.data() + old_size, &crc_value, sizeof(uint32_t));

                    // 更新 payload_len
                    f.header.payload_len = static_cast<uint32_t>(f.payload.size());
                }

                return f;
            }

            /**
             * @brief 编码普通消息（自动分包）
             * @param message_id 消息唯一标识符
             * @param data 消息数据
             * @param length 数据长度
             * @param enable_crc 是否启用 CRC32 校验
             * @return 编码后的帧数组
             * @throw ProtocolError 消息过大时抛出异常
             */
            std::vector<Frame> Encoder::EncodeMessage(
                uint64_t message_id,
                const uint8_t *data,
                size_t length,
                bool enable_crc)
            {
                const size_t slice_payload =
                    MAX_FRAME_PAYLOAD - sizeof(MessageHeader) - (enable_crc ? sizeof(uint32_t) : 0);

                uint16_t total =
                    static_cast<uint16_t>((length + slice_payload - 1) / slice_payload);

                if (total == 0 || total > MAX_MESSAGE_SLICES)
                    throw ProtocolError("message too large");

                std::vector<Frame> frames;
                frames.reserve(total);

                for (uint16_t i = 0; i < total; ++i)
                {
                    size_t offset = i * slice_payload;
                    size_t chunk = std::min(slice_payload, length - offset);

                    std::vector<uint8_t> payload(sizeof(MessageHeader) + chunk);
                    auto *hdr = reinterpret_cast<MessageHeader *>(payload.data());
                    hdr->message_id = message_id;
                    hdr->total_slices = total;
                    hdr->sequence = i;

                    std::memcpy(payload.data() + sizeof(MessageHeader),
                                data + offset,
                                chunk);

                    frames.push_back(
                        MakeFrame(FrameType::Message, payload.data(), payload.size(), enable_crc));
                }
                return frames;
            }

            /**
             * @brief 编码流开始帧
             * @param stream_id 流唯一标识符
             * @param total_size 流的总大小（0 表示未知）
             * @return 流开始帧
             */
            Frame Encoder::EncodeStreamStart(uint64_t stream_id, uint64_t total_size)
            {
                StreamStartPayload p{stream_id, total_size};
                return MakeFrame(FrameType::StreamStart, &p, sizeof(p));
            }

            /**
             * @brief 编码流数据块帧
             * @param stream_id 流唯一标识符
             * @param offset 数据在流中的偏移量
             * @param data 数据
             * @param length 数据长度
             * @return 流数据块帧
             * @throw ProtocolError 数据块过大时抛出异常
             */
            Frame Encoder::EncodeStreamChunk(
                uint64_t stream_id,
                uint64_t offset,
                const uint8_t *data,
                size_t length)
            {
                if (length + sizeof(StreamChunkPayload) > MAX_FRAME_PAYLOAD)
                    throw ProtocolError("stream chunk too large");

                std::vector<uint8_t> payload(sizeof(StreamChunkPayload) + length);
                auto *p = reinterpret_cast<StreamChunkPayload *>(payload.data());
                p->stream_id = stream_id;
                p->offset = offset;

                std::memcpy(payload.data() + sizeof(StreamChunkPayload),
                            data, length);

                return MakeFrame(FrameType::StreamChunk, payload.data(), payload.size());
            }

            /**
             * @brief 编码流结束帧
             * @param stream_id 流唯一标识符
             * @param crc32 整个流的 CRC32 校验值（0 表示不校验）
             * @return 流结束帧
             */
            Frame Encoder::EncodeStreamEnd(uint64_t stream_id, uint32_t crc32)
            {
                StreamEndPayload p{stream_id, crc32};
                return MakeFrame(FrameType::StreamEnd, &p, sizeof(p));
            }

            /**
             * @brief 将 Frame 数组序列化为字节流数组
             * @param frames 帧数组
             * @return 序列化后的字节流数组
             */
            std::vector<std::vector<uint8_t>> Encoder::SerializeFrames(
                const std::vector<Frame> &frames)
            {
                std::vector<std::vector<uint8_t>> result;
                result.reserve(frames.size());

                for (const auto &frame : frames)
                {
                    result.push_back(frame.Serialize());
                }

                return result;
            }

            // ========== Decoder 实现 ==========

            /**
             * @brief 构造解码器
             * @param message_timeout_ms 消息组装超时时间（毫秒）
             */
            Decoder::Decoder(uint32_t message_timeout_ms)
                : message_timeout_ms_(message_timeout_ms)
            {
            }

            /**
             * @brief 喂入数据（支持多次调用，自动处理粘包）
             * @param data 数据指针
             * @param len 数据长度
             */
            void Decoder::Feed(const uint8_t *data, size_t len)
            {
                stats_.bytes_received += len;
                buffer_.insert(buffer_.end(), data, data + len);
                TryDecode();
            }

            /**
             * @brief 尝试从缓冲区解码数据
             * @throw ProtocolError 协议错误时抛出异常
             */
            void Decoder::TryDecode()
            {
                while (true)
                {
                    stats_.buffer_size = buffer_.size();

                    if (buffer_.size() < sizeof(FrameHeader))
                        return;

                    auto *hdr = reinterpret_cast<FrameHeader *>(buffer_.data());

                    // 验证魔数
                    if (hdr->magic1 != MAGIC1 || hdr->magic2 != MAGIC2)
                        throw ProtocolError("bad magic");

                    // 验证版本
                    if (hdr->version != VERSION)
                        throw ProtocolError("unsupported version");

                    if (hdr->payload_len > MAX_FRAME_PAYLOAD)
                        throw ProtocolError("payload too large");

                    size_t frame_size = sizeof(FrameHeader) + hdr->payload_len;
                    if (buffer_.size() < frame_size)
                        return;

                    const uint8_t *payload =
                        buffer_.data() + sizeof(FrameHeader);

                    FrameType type = static_cast<FrameType>(hdr->type);
                    stats_.frames_received++;

                    // 处理 CRC32 校验
                    bool has_crc = (hdr->flags & FLAG_CRC32) != 0;
                    size_t payload_data_len = hdr->payload_len;

                    if (has_crc && hdr->payload_len >= sizeof(uint32_t))
                    {
                        payload_data_len -= sizeof(uint32_t);

                        // 验证 CRC
                        uint32_t received_crc;
                        std::memcpy(&received_crc, payload + payload_data_len, sizeof(uint32_t));

                        uint32_t calculated_crc = CalculateCRC32(payload, payload_data_len);
                        if (received_crc != calculated_crc)
                        {
                            stats_.crc_errors++;
                            // 跳过这个错误的帧
                            buffer_.erase(buffer_.begin(), buffer_.begin() + frame_size);
                            continue;
                        }
                    }

                    if (type == FrameType::Message)
                    {
                        auto *mh = reinterpret_cast<const MessageHeader *>(payload);
                        if (mh->sequence >= mh->total_slices)
                            throw ProtocolError("bad slice index");

                        auto &m = messages_[mh->message_id];
                        if (m.slices.empty())
                        {
                            m.total = mh->total_slices;
                            m.slices.resize(m.total);
                            m.first_seen = std::chrono::steady_clock::now();
                        }

                        auto &slot = m.slices[mh->sequence];
                        if (slot.empty())
                        {
                            slot.assign(payload + sizeof(MessageHeader),
                                        payload + payload_data_len);
                            m.received++;
                        }

                        if (m.received == m.total)
                        {
                            MessageComplete msg;
                            msg.message_id = mh->message_id;
                            msg.data.reserve(m.total * MAX_FRAME_PAYLOAD);

                            for (auto &s : m.slices)
                                msg.data.insert(msg.data.end(), s.begin(), s.end());

                            completed_messages_.push_back(std::move(msg));
                            messages_.erase(mh->message_id);
                            stats_.messages_completed++;
                        }
                    }
                    else
                    {
                        stats_.stream_events++;
                        StreamEvent ev{};
                        ev.type = type;

                        if (type == FrameType::StreamChunk)
                        {
                            auto *sc =
                                reinterpret_cast<const StreamChunkPayload *>(payload);
                            ev.stream_id = sc->stream_id;
                            ev.offset = sc->offset;
                            ev.data.assign(payload + sizeof(StreamChunkPayload),
                                           payload + payload_data_len);
                        }
                        else if (type == FrameType::StreamStart)
                        {
                            auto *ss =
                                reinterpret_cast<const StreamStartPayload *>(payload);
                            ev.stream_id = ss->stream_id;
                            ev.total_size = ss->total_size;
                        }
                        else if (type == FrameType::StreamEnd)
                        {
                            auto *se =
                                reinterpret_cast<const StreamEndPayload *>(payload);
                            ev.stream_id = se->stream_id;
                            ev.crc32 = se->crc32;
                        }
                        stream_events_.push_back(std::move(ev));
                    }

                    buffer_.erase(buffer_.begin(), buffer_.begin() + frame_size);
                }
            }

            /**
             * @brief 获取完整的消息
             * @param out 输出消息
             * @return 有消息返回 true，无消息返回 false
             */
            bool Decoder::GetMessage(MessageComplete &out)
            {
                if (completed_messages_.empty())
                    return false;
                out = std::move(completed_messages_.front());
                completed_messages_.erase(completed_messages_.begin());
                return true;
            }

            /**
             * @brief 获取流事件
             * @param out 输出流事件
             * @return 有事件返回 true，无事件返回 false
             */
            bool Decoder::GetStreamEvent(StreamEvent &out)
            {
                if (stream_events_.empty())
                    return false;
                out = std::move(stream_events_.front());
                stream_events_.erase(stream_events_.begin());
                return true;
            }

            /**
             * @brief 获取统计信息
             * @return 统计信息结构
             */
            DecoderStats Decoder::GetStats() const
            {
                DecoderStats stats = stats_;
                stats.pending_messages = messages_.size();
                stats.buffer_size = buffer_.size();
                return stats;
            }

            /**
             * @brief 清理超时的消息
             * @return 清理的消息数量
             */
            size_t Decoder::CleanupTimeoutMessages()
            {
                size_t cleaned = 0;
                auto now = std::chrono::steady_clock::now();

                auto it = messages_.begin();
                while (it != messages_.end())
                {
                    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   now - it->second.first_seen)
                                   .count();

                    if (age >= message_timeout_ms_)
                    {
                        it = messages_.erase(it);
                        cleaned++;
                        stats_.timeout_cleanups++;
                    }
                    else
                    {
                        ++it;
                    }
                }

                return cleaned;
            }

            /**
             * @brief 清空所有状态（重置解码器）
             */
            void Decoder::Reset()
            {
                buffer_.clear();
                messages_.clear();
                completed_messages_.clear();
                stream_events_.clear();
                stats_ = DecoderStats{};
            }

            // ========== CRC32 工具函数实现 ==========

            /**
             * @brief 计算 CRC32 校验值（标准多项式：0xEDB88320）
             * @param data 数据指针
             * @param len 数据长度
             * @return CRC32 校验值
             */
            uint32_t CalculateCRC32(const uint8_t *data, size_t len)
            {
                static const uint32_t CRC_TABLE[256] = {
                    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
                    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
                    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
                    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
                    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
                    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
                    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
                    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
                    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
                    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
                    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
                    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
                    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
                    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
                    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
                    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
                    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
                    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
                    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
                    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
                    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
                    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
                    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
                    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
                    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
                    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
                    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
                    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
                    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
                    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
                    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
                    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
                    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
                    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
                    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
                    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
                    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
                    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
                    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
                    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
                    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
                    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
                    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

                uint32_t crc = 0xFFFFFFFF;
                for (size_t i = 0; i < len; ++i)
                {
                    crc = CRC_TABLE[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
                }
                return crc ^ 0xFFFFFFFF;
            }

        } // namespace proto
    } // namespace network
} // namespace darwincore
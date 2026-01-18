#ifndef DARWINCORE_NETWORK_CONNECTION_ID_GENERATORs_H
#define DARWINCORE_NETWORK_CONNECTION_ID_GENERATORs_H

#include <cstdint>

namespace darwincore
{
    namespace network
    {
        class ConnectionIdGenerator
        {
        public:
            // 格式: [24位 YYMMDD][8位 ReactorId][16位 Fd][16位 Seq]
            static uint64_t Generate(uint8_t reactorId, uint16_t fd, uint16_t seq);

            // 解析完整信息
            static void Parse(uint64_t connId,
                              uint32_t &date,
                              uint8_t &reactorId,
                              uint16_t &fd,
                              uint16_t &seq);

            // 快速字段访问
            static uint32_t GetDate(uint64_t connId);
            static uint8_t GetReactorId(uint64_t connId);
            static uint16_t GetFd(uint64_t connId);
            static uint16_t GetSeq(uint64_t connId);

        private:
            // 返回 YYMMDD（本地时间）
            static uint32_t GetCurrentDate();
        };
    };
};

#endif
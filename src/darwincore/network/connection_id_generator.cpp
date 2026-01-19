#include "connection_id_generator.h"

#include <ctime>

using namespace darwincore::network;

uint64_t ConnectionIdGenerator::Generate(uint8_t reactorId,
                                         uint16_t fd,
                                         uint16_t seq)
{
    uint32_t dateShort = GetCurrentDate(); // YYMMDD

    uint64_t connectionId =
        (static_cast<uint64_t>(dateShort & 0xFFFFFF) << 40) |
        (static_cast<uint64_t>(reactorId & 0xFF) << 32) |
        (static_cast<uint64_t>(fd & 0xFFFF) << 16) |
        (static_cast<uint64_t>(seq & 0xFFFF));

    return connectionId;
}

void ConnectionIdGenerator::Parse(uint64_t connId,
                                  uint32_t &date,
                                  uint8_t &reactorId,
                                  uint16_t &fd,
                                  uint16_t &seq)
{
    date = (connId >> 40) & 0xFFFFFF;
    reactorId = (connId >> 32) & 0xFF;
    fd = (connId >> 16) & 0xFFFF;
    seq = connId & 0xFFFF;
}

uint32_t ConnectionIdGenerator::GetDate(uint64_t connId)
{
    return (connId >> 40) & 0xFFFFFF;
}

uint8_t ConnectionIdGenerator::GetReactorId(uint64_t connId)
{
    return (connId >> 32) & 0xFF;
}

uint16_t ConnectionIdGenerator::GetFd(uint64_t connId)
{
    return (connId >> 16) & 0xFFFF;
}

uint16_t ConnectionIdGenerator::GetSeq(uint64_t connId)
{
    return connId & 0xFFFF;
}

uint32_t ConnectionIdGenerator::GetCurrentDate()
{
    std::time_t now = std::time(nullptr);

    std::tm tmInfo{};
    localtime_r(&now, &tmInfo);

    return static_cast<uint32_t>(
        (tmInfo.tm_year % 100) * 10000 +
        (tmInfo.tm_mon + 1) * 100 +
        tmInfo.tm_mday);
}
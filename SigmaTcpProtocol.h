#pragma once
#include <Arduino.h>

#pragma pack(push, 1)

constexpr uint8_t SIGMA_WRITE_REQUEST  = 0x09;
constexpr uint8_t SIGMA_READ_REQUEST   = 0x0A;
constexpr uint8_t SIGMA_READ_RESPONSE  = 0x0B;

struct SigmaPacketHeader {
    uint8_t  control;
    uint32_t totalLength;
    uint8_t  chipAddress;
    uint32_t dataLength;
    uint16_t address;
};

struct SigmaWriteRequest {
    uint8_t  control;
    uint8_t  blockOrSafeload;
    uint8_t  channelNumber;
    uint32_t totalLength;
    uint8_t  chipAddress;
    uint32_t dataLength;
    uint16_t address;
};

struct SigmaReadRequest {
    uint8_t  control;
    uint32_t totalLength;
    uint8_t  chipAddress;
    uint32_t dataLength;
    uint16_t address;
    uint8_t  reserved0;
    uint8_t  reserved1;
};

struct SigmaReadResponse {
    uint8_t  control;
    uint32_t totalLength;
    uint8_t  chipAddress;
    uint32_t dataLength;
    uint16_t address;
    uint8_t  status;
    uint8_t  reserved;
};

#pragma pack(pop)

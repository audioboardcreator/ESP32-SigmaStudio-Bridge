#pragma once
#include <Arduino.h>
#include "driver/spi_master.h"
#include "TcpMessageQueue.h"

class Adau1467 {
public:
    Adau1467(TcpMessageQueue& receiveQueue, TcpMessageQueue& transmitQueue);
    bool begin(spi_host_device_t spiHost, int mosiPin, int misoPin, int clockPin, int chipSelectPin, uint32_t clockFrequency, BaseType_t taskCore, UBaseType_t taskPriority);
    bool writeSpi(uint16_t address, const uint8_t* data, uint32_t length);
    bool readSpi(uint16_t address, uint8_t* data, uint32_t length);

private:
    struct WriteRequestHeader {
        uint8_t control;
        uint8_t blockOrSafeload;
        uint8_t channel;
        uint32_t totalLength;
        uint8_t chipAddress;
        uint32_t dataLength;
        uint16_t address;
    };

    struct ReadRequestHeader {
        uint8_t control;
        uint32_t totalLength;
        uint8_t chipAddress;
        uint32_t dataLength;
        uint16_t address;
        uint8_t reserved0;
        uint8_t reserved1;
    };

    static void taskEntry(void* parameter);
    void task();
    void processMessage(const TcpMessage& message);
    void processWriteRequest(const TcpMessage& message);
    void processReadRequest(const TcpMessage& message);
    bool initializeSpi(spi_host_device_t spiHost, int mosiPin, int misoPin, int clockPin, int chipSelectPin, uint32_t clockFrequency);
    bool transmitRaw(const uint8_t* transmitData, uint8_t* receiveData, size_t length, bool keepChipSelectActive);
    bool decodeWriteHeader(const TcpMessage& message, WriteRequestHeader& header) const;
    bool decodeReadRequestHeader(const TcpMessage& message, ReadRequestHeader& header) const;
    TcpMessage createReadResponse(const ReadRequestHeader& request);
    uint16_t decodeBigEndian16(const uint8_t* data) const;
    uint32_t decodeBigEndian32(const uint8_t* data) const;
    void encodeBigEndian16(uint8_t* data, uint16_t value) const;
    void encodeBigEndian32(uint8_t* data, uint32_t value) const;

    TcpMessageQueue& _receiveQueue;
    TcpMessageQueue& _transmitQueue;
    TaskHandle_t _taskHandle;
    spi_host_device_t _spiHost;
    spi_device_handle_t _spiDevice;
    uint8_t* _dmaTransmitBuffer;
    uint8_t* _dmaReceiveBuffer;

    static constexpr uint8_t CONTROL_WRITE_REQUEST = 0x09;
    static constexpr uint8_t CONTROL_READ_REQUEST = 0x0A;
    static constexpr uint8_t CONTROL_READ_RESPONSE = 0x0B;
    static constexpr uint8_t SPI_WRITE_COMMAND = 0x00;
    static constexpr uint8_t SPI_READ_COMMAND = 0x01;
    static constexpr uint32_t WRITE_HEADER_SIZE = 14;
    static constexpr uint32_t READ_REQUEST_SIZE = 14;
    static constexpr uint32_t READ_RESPONSE_HEADER_SIZE = 14;
    static constexpr size_t SPI_COMMAND_SIZE = 3;
    static constexpr size_t SPI_DMA_BUFFER_SIZE = 4096;
    static constexpr uint32_t MAX_DATA_SIZE = 100UL * 1024UL;
};
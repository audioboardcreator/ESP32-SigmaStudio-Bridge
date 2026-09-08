#include "Adau1467.h"
#include "TcpRecorder.h"
#include "Config.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "DualConsole.h"

Adau1467::Adau1467(TcpMessageQueue& receiveQueue, TcpMessageQueue& transmitQueue)
  : _receiveQueue(receiveQueue), _transmitQueue(transmitQueue), _taskHandle(nullptr), _spiHost(SPI2_HOST), _spiDevice(nullptr), _dmaTransmitBuffer(nullptr), _dmaReceiveBuffer(nullptr) {}

bool Adau1467::begin(spi_host_device_t spiHost, int mosiPin, int misoPin, int clockPin, int chipSelectPin, uint32_t clockFrequency, BaseType_t taskCore, UBaseType_t taskPriority) {
  if (_taskHandle != nullptr) return true;
  if (!_receiveQueue.isReady() || !_transmitQueue.isReady()) {
    Serial.println("Error: ADAU queues are not initialized");
    return false;
  }
  if (!initializeSpi(spiHost, mosiPin, misoPin, clockPin, chipSelectPin, clockFrequency)) return false;
  BaseType_t result = xTaskCreatePinnedToCore(taskEntry, "Adau1467Task", 8192, this, taskPriority, &_taskHandle, taskCore);
  if (result != pdPASS) {
    Serial.println("Error: Failed to start ADAU1467 task");
    _taskHandle = nullptr;
    return false;
  }
  Serial.printf("ADAU1467 task started on Core %d\n", static_cast<int>(taskCore));
  return true;
}

bool Adau1467::initializeSpi(spi_host_device_t spiHost, int mosiPin, int misoPin, int clockPin, int chipSelectPin, uint32_t clockFrequency) {
  _spiHost = spiHost;
  _dmaTransmitBuffer = static_cast<uint8_t*>(heap_caps_malloc(SPI_DMA_BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
  _dmaReceiveBuffer = static_cast<uint8_t*>(heap_caps_malloc(SPI_DMA_BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
  if (_dmaTransmitBuffer == nullptr || _dmaReceiveBuffer == nullptr) {
    Serial.println("Error: Failed to allocate SPI DMA buffers");
    if (_dmaTransmitBuffer != nullptr) { free(_dmaTransmitBuffer); _dmaTransmitBuffer = nullptr; }
    if (_dmaReceiveBuffer != nullptr) { free(_dmaReceiveBuffer); _dmaReceiveBuffer = nullptr; }
    return false;
  }

  spi_bus_config_t busConfiguration{};
  busConfiguration.mosi_io_num = mosiPin;
  busConfiguration.miso_io_num = misoPin;
  busConfiguration.sclk_io_num = clockPin;
  busConfiguration.quadwp_io_num = -1;
  busConfiguration.quadhd_io_num = -1;
  busConfiguration.max_transfer_sz = SPI_DMA_BUFFER_SIZE;
  esp_err_t result = spi_bus_initialize(_spiHost, &busConfiguration, SPI_DMA_CH_AUTO);
  if (result != ESP_OK) {
    Serial.printf("spi_bus_initialize failed: %s\n", esp_err_to_name(result));
    free(_dmaTransmitBuffer); free(_dmaReceiveBuffer);
    _dmaTransmitBuffer = nullptr; _dmaReceiveBuffer = nullptr;
    return false;
  }

  spi_device_interface_config_t deviceConfiguration{};
  deviceConfiguration.clock_speed_hz = clockFrequency;
  deviceConfiguration.mode = 3;
  deviceConfiguration.spics_io_num = chipSelectPin;
  deviceConfiguration.queue_size = 2;
  deviceConfiguration.flags = 0;
  result = spi_bus_add_device(_spiHost, &deviceConfiguration, &_spiDevice);
  if (result != ESP_OK) {
    Serial.printf("spi_bus_add_device failed: %s\n", esp_err_to_name(result));
    spi_bus_free(_spiHost);
    free(_dmaTransmitBuffer); free(_dmaReceiveBuffer);
    _dmaTransmitBuffer = nullptr; _dmaReceiveBuffer = nullptr;
    return false;
  }
  Serial.printf("SPI initialized: %lu Hz, Mode 3\n", static_cast<unsigned long>(clockFrequency));
  return true;
}

void Adau1467::taskEntry(void* parameter) {
  Adau1467* instance = static_cast<Adau1467*>(parameter);
  instance->task();
}

void Adau1467::task() {
  while (true) {
    TcpMessage message{ 0, nullptr };
    bool received = _receiveQueue.receive(message, portMAX_DELAY);
    if (!received) continue;
    if (message.data != nullptr && message.length > 0) processMessage(message);
    else Serial.println("Received empty ADAU message");
    _receiveQueue.release(message);
  }
}

void Adau1467::processMessage(const TcpMessage& message) {
  // WICHTIGER FIX: message.data[0] statt message.data prüfen!
  switch (message.data[0]) {
    case CONTROL_WRITE_REQUEST: processWriteRequest(message); break;
    case CONTROL_READ_REQUEST: processReadRequest(message); break;
    default: Serial.printf("Unknown ADAU command: 0x%02X\n", message.data[0]); break;
  }
}

void Adau1467::processWriteRequest(const TcpMessage& message) {
  WriteRequestHeader header{};
  if (!decodeWriteHeader(message, header)) {
    Serial.println("Invalid Write Request");
    return;
  }
  uint32_t availableData = message.length - WRITE_HEADER_SIZE;
  if (header.dataLength > availableData || header.dataLength > MAX_DATA_SIZE) {
    Serial.println("Write Request: Payload error or size limits exceeded");
    return;
  }
  const uint8_t* payload = message.data + WRITE_HEADER_SIZE;
  writeSpi(header.address, payload, header.dataLength);
}

void Adau1467::processReadRequest(const TcpMessage& message) {
  ReadRequestHeader request{};
  if (!decodeReadRequestHeader(message, request) || request.dataLength > MAX_DATA_SIZE) {
    Serial.println("Invalid or oversized Read Request");
    return;
  }
  TcpMessage response = createReadResponse(request);
  if (response.data == nullptr) return;
  uint8_t* responsePayload = response.data + READ_RESPONSE_HEADER_SIZE;
  bool success = readSpi(request.address, responsePayload, request.dataLength);
  response.data[12] = success ? 0x00 : 0x01; // FIX: Array-Index
  if (!success) memset(responsePayload, 0, request.dataLength);
  if (!_transmitQueue.send(response, pdMS_TO_TICKS(1000))) {
    _transmitQueue.release(response);
  }
}

bool Adau1467::writeSpi(uint16_t address, const uint8_t* data, uint32_t length) {
  if (_spiDevice == nullptr || data == nullptr || length == 0) return false;
  neopixelWrite(PIN_ONBOARD_LED, 0, 32, 0);
  tcpRecorder.recordSpiWrite(address, data, length);
  if (spi_device_acquire_bus(_spiDevice, portMAX_DELAY) != ESP_OK) return false;

  uint32_t offset = 0;
  bool firstBlock = true;
  bool success = true;

  while (offset < length) {
    size_t headerLength = firstBlock ? SPI_COMMAND_SIZE : 0;
    size_t maximumPayload = SPI_DMA_BUFFER_SIZE - headerLength;
    size_t chunkLength = std::min(maximumPayload, static_cast<size_t>(length - offset));
    if (firstBlock) {
      _dmaTransmitBuffer[0] = SPI_WRITE_COMMAND;
      _dmaTransmitBuffer[1] = static_cast<uint8_t>(address >> 8);
      _dmaTransmitBuffer[2] = static_cast<uint8_t>(address & 0xFF);
    }
    memcpy(_dmaTransmitBuffer + headerLength, data + offset, chunkLength);
    bool finalBlock = offset + chunkLength >= length;
    if (!transmitRaw(_dmaTransmitBuffer, nullptr, headerLength + chunkLength, !finalBlock)) {
      success = false;
      break;
    }
    offset += static_cast<uint32_t>(chunkLength);
    firstBlock = false;
  }
  spi_device_release_bus(_spiDevice);

  if (success) {
    uint32_t delayMs = 0;
    switch (address) {
      case 0xF890: if (length >= 2 && data[1] == 0x01) delayMs = 255; break;
      case 0xF400: if (length >= 2 && data[1] == 0x01) delayMs = 255; break;
      case 0xF003: if (length >= 2 && data[1] == 0x01) delayMs = 255; break;
      case 0xF402: if (length >= 2 && data[1] == 0x01) delayMs = 10; break;
      default: delayMicroseconds(10); break;
    }
    if (delayMs > 0) {
      Serial.printf("[TIMING] Detected trigger on Addr 0x%04X. Waiting %lu ms...\n", address, delayMs);
      delay(delayMs);
    }
  }
  return success;
}

bool Adau1467::readSpi(uint16_t address, uint8_t* data, uint32_t length) {
  if (_spiDevice == nullptr || data == nullptr || length == 0) return false;
  neopixelWrite(PIN_ONBOARD_LED, 0, 32, 0);
  if (spi_device_acquire_bus(_spiDevice, portMAX_DELAY) != ESP_OK) return false;

  uint32_t offset = 0;
  bool firstBlock = true;
  bool success = true;

  while (offset < length) {
    size_t headerLength = firstBlock ? SPI_COMMAND_SIZE : 0;
    size_t chunkLength = std::min(SPI_DMA_BUFFER_SIZE - headerLength, static_cast<size_t>(length - offset));
    size_t transactionLength = headerLength + chunkLength;
    memset(_dmaTransmitBuffer, 0, transactionLength);
    memset(_dmaReceiveBuffer, 0, transactionLength);
    if (firstBlock) {
      _dmaTransmitBuffer[0] = SPI_READ_COMMAND;
      _dmaTransmitBuffer[1] = static_cast<uint8_t>(address >> 8);
      _dmaTransmitBuffer[2] = static_cast<uint8_t>(address & 0xFF);
    }
    bool finalBlock = offset + chunkLength >= length;
    if (!transmitRaw(_dmaTransmitBuffer, _dmaReceiveBuffer, transactionLength, !finalBlock)) {
      success = false;
      break;
    }
    memcpy(data + offset, _dmaReceiveBuffer + headerLength, chunkLength);
    offset += static_cast<uint32_t>(chunkLength);
    firstBlock = false;
  }
  spi_device_release_bus(_spiDevice);
  return success;
}

bool Adau1467::transmitRaw(const uint8_t* transmitData, uint8_t* receiveData, size_t length, bool keepChipSelectActive) {
  spi_transaction_t transaction{};
  transaction.length = length * 8;
  transaction.rxlength = receiveData != nullptr ? length * 8 : 0;
  transaction.tx_buffer = transmitData;
  transaction.rx_buffer = receiveData;
  if (keepChipSelectActive) transaction.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
  if (spi_device_polling_transmit(_spiDevice, &transaction) != ESP_OK) return false;
  return true;
}

TcpMessage Adau1467::createReadResponse(const ReadRequestHeader& request) {
  TcpMessage response{ 0, nullptr };
  uint32_t totalLength = READ_RESPONSE_HEADER_SIZE + request.dataLength;
  response.data = static_cast<uint8_t*>(heap_caps_malloc(totalLength, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (response.data == nullptr) return response;
  response.length = totalLength;
  memset(response.data, 0, totalLength);
  response.data[0] = CONTROL_READ_RESPONSE;
  encodeBigEndian32(&response.data[1], totalLength);
  response.data[5] = request.chipAddress;
  encodeBigEndian32(&response.data[6], request.dataLength);
  encodeBigEndian16(&response.data[10], request.address);
  response.data[12] = 0x01;
  response.data[13] = 0x00;
return response;
}
bool Adau1467::decodeWriteHeader(const TcpMessage& message, WriteRequestHeader& header) const {
if (message.data == nullptr || message.length < WRITE_HEADER_SIZE || message.data[0] != CONTROL_WRITE_REQUEST) return false;
header.control = message.data[0];
header.blockOrSafeload = message.data[1];
header.channel = message.data[2];
header.totalLength = decodeBigEndian32(&message.data[3]);
header.chipAddress = message.data[7];
header.dataLength = decodeBigEndian32(&message.data[8]);
header.address = decodeBigEndian16(&message.data[12]);
return header.totalLength == message.length && header.dataLength <= message.length - WRITE_HEADER_SIZE;
}
bool Adau1467::decodeReadRequestHeader(const TcpMessage& message, ReadRequestHeader& header) const {
if (message.data == nullptr || message.length < READ_REQUEST_SIZE || message.data[0] != CONTROL_READ_REQUEST) return false;
header.control = message.data[0];
header.totalLength = decodeBigEndian32(&message.data[1]);
header.chipAddress = message.data[5];
header.dataLength = decodeBigEndian32(&message.data[6]);
header.address = decodeBigEndian16(&message.data[10]);
header.reserved0 = message.data[12];
header.reserved1 = message.data[13];
return header.totalLength == message.length;
}
uint16_t Adau1467::decodeBigEndian16(const uint8_t* data) const {
return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
}
uint32_t Adau1467::decodeBigEndian32(const uint8_t* data) const {
return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) | (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
}
void Adau1467::encodeBigEndian16(uint8_t* data, uint16_t value) const {
data[0] = static_cast<uint8_t>(value >> 8);
data[1] = static_cast<uint8_t>(value);
}
void Adau1467::encodeBigEndian32(uint8_t* data, uint32_t value) const {
data[0] = static_cast<uint8_t>(value >> 24);
data[1] = static_cast<uint8_t>(value >> 16);
data[2] = static_cast<uint8_t>(value >> 8);
data[3] = static_cast<uint8_t>(value);
}


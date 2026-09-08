#include "HardwareSerial.h"
#include "TcpConnectionManager.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include "TcpLogger.h"
#include "TcpRecorder.h"
#include "esp_heap_caps.h"
#include "DualConsole.h"

TcpConnectionManager::TcpConnectionManager(uint16_t port, TcpMessageQueue& receiveQueue, TcpMessageQueue& transmitQueue)
  : _server(port), _receiveQueue(receiveQueue), _transmitQueue(transmitQueue), _taskHandle(nullptr), _receiveState(ReceiveState::ReadPreview), _previewReceived(0), _messageBuffer(nullptr), _messageLength(0), _messageReceived(0) {
  memset(_preview, 0, sizeof(_preview));
}

bool TcpConnectionManager::begin(const char* ssid, const char* password, BaseType_t core, UBaseType_t priority) {
  if (_taskHandle != nullptr) return true;
  if (!_receiveQueue.isReady() || !_transmitQueue.isReady()) { Serial.println("Error: TCP queues are not initialized"); return false; }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(); Serial.println("Wi-Fi connected");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  _server.begin();
  BaseType_t result = xTaskCreatePinnedToCore(taskEntry, "TcpIoTask", 8192, this, priority, &_taskHandle, core);
  if (result != pdPASS) { Serial.println("Error: Failed to start TCP I/O task"); _taskHandle = nullptr; return false; }
  Serial.printf("TCP I/O task started on Core %d\n", static_cast<int>(core));
  return true;
}

bool TcpConnectionManager::connected() { return _client && _client.connected(); }

void TcpConnectionManager::taskEntry(void* parameter) {
  TcpConnectionManager* instance = static_cast<TcpConnectionManager*>(parameter);
  instance->task();
}

void TcpConnectionManager::task() {
  while (true) {
    handleDisconnect();
    acceptClient();
    if (_client && _client.connected()) { processIncomingData(); processOutgoingData(); }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void TcpConnectionManager::acceptClient() {
  if (_client && _client.connected()) return;
  WiFiClient newClient = _server.available();
  if (!newClient) return;
  _client = newClient;
  resetReceiver();
  Serial.print("Client connected: "); Serial.println(_client.remoteIP());
}

void TcpConnectionManager::handleDisconnect() {
    if (!_client) return;
    if (_client.connected()) return;
    Serial.println("Client disconnected");
    Serial.printf("Available bytes = %d\n", _client.available());
    _client.stop();
    resetReceiver();
}

void TcpConnectionManager::processIncomingData() {
  while (_client.connected() && _client.available() > 0) {
    switch (_receiveState) {
      case ReceiveState::ReadPreview: readPreview(); break;
      case ReceiveState::ReadMessage: readMessageBody(); break;
    }
  }
}

void TcpConnectionManager::processOutgoingData() {
  TcpMessage message{0, nullptr};
  if (!_transmitQueue.receive(message, 0)) return;
  tcpLogger.log(LogType::TX, message.data, message.length); 
  bool success = sendCompleteMessage(message);
  if (!success) Serial.println("Failed to send TCP response");
  _transmitQueue.release(message);
}

bool TcpConnectionManager::sendCompleteMessage(const TcpMessage& message) {
  if (!_client || !_client.connected() || message.data == nullptr || message.length == 0) return false;
  uint32_t sentBytes = 0;
  while (sentBytes < message.length) {
    if (!_client.connected()) return false;
    size_t remainingBytes = static_cast<size_t>(message.length - sentBytes);
    size_t chunkLength = std::min(NETWORK_CHUNK_SIZE, remainingBytes);
    size_t result = _client.write(message.data + sentBytes, chunkLength);
    if (result == 0) { vTaskDelay(pdMS_TO_TICKS(1)); continue; }
    sentBytes += static_cast<uint32_t>(result);
  }
  return true;
}

void TcpConnectionManager::readPreview() {
  if (_previewReceived == 0) {
    int value = _client.read();
    if (value < 0) return;
    uint8_t control = static_cast<uint8_t>(value);
    if (!isKnownControlByte(control)) { Serial.printf("Unknown Control-Byte dropped: 0x%02X\n", control); return; }
    _preview[0] = control;
    _previewReceived = 1;
  }
  if (_previewReceived >= PREVIEW_SIZE) { prepareMessage(); return; }
  int availableBytes = _client.available();
  if (availableBytes <= 0) return;
  size_t missingBytes = PREVIEW_SIZE - _previewReceived;
  size_t bytesToRead = std::min(missingBytes, static_cast<size_t>(availableBytes));
  int bytesRead = _client.read(_preview + _previewReceived, bytesToRead);
  if (bytesRead > 0) _previewReceived += static_cast<size_t>(bytesRead);
  if (_previewReceived == PREVIEW_SIZE) prepareMessage();
}

void TcpConnectionManager::prepareMessage() {
  _messageLength = getTotalLength(_preview);
  if (_messageLength < PREVIEW_SIZE) { Serial.println("Error: Message size is smaller than preview size"); _client.stop(); resetReceiver(); return; }
  if (_messageLength > MAX_MESSAGE_SIZE) { Serial.printf("Error: Message too large: %lu Bytes\n", static_cast<unsigned long>(_messageLength)); _client.stop(); resetReceiver(); return; }
  _messageBuffer = static_cast<uint8_t*>(heap_caps_malloc(_messageLength, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (_messageBuffer == nullptr) { Serial.printf("Error: PSRAM allocation failed for %lu Bytes\n", static_cast<unsigned long>(_messageLength)); _client.stop(); resetReceiver(); return; }
  memcpy(_messageBuffer, _preview, PREVIEW_SIZE);
  _messageReceived = PREVIEW_SIZE;
  _receiveState = ReceiveState::ReadMessage;
  if (_messageReceived == _messageLength) finishMessage();
}

void TcpConnectionManager::readMessageBody() {
  if (_messageBuffer == nullptr) { resetReceiver(); return; }
  if (_messageReceived >= _messageLength) { finishMessage(); return; }
  int availableBytes = _client.available();
  if (availableBytes <= 0) return;
  uint32_t missingBytes = _messageLength - _messageReceived;
  size_t bytesToRead = std::min(static_cast<size_t>(missingBytes), static_cast<size_t>(availableBytes));
  bytesToRead = std::min(bytesToRead, NETWORK_CHUNK_SIZE);
  int bytesRead = _client.read(_messageBuffer + _messageReceived, bytesToRead);
  if (bytesRead <= 0) return;
  _messageReceived += static_cast<uint32_t>(bytesRead);
  if (_messageReceived == _messageLength) finishMessage();
}

void TcpConnectionManager::finishMessage() {
  TcpMessage message{_messageLength, _messageBuffer};
  tcpLogger.log(LogType::RX, message.data, message.length);
  bool queued = _receiveQueue.send(message, pdMS_TO_TICKS(1000));
  if (!queued) { Serial.println("RX Queue full, message dropped"); free(_messageBuffer); }
  _messageBuffer = nullptr; _messageLength = 0; _messageReceived = 0; _previewReceived = 0;
  _receiveState = ReceiveState::ReadPreview;
  memset(_preview, 0, sizeof(_preview));
}

void TcpConnectionManager::resetReceiver() {
  if (_messageBuffer != nullptr) { free(_messageBuffer); _messageBuffer = nullptr; }
  _messageLength = 0; _messageReceived = 0; _previewReceived = 0;
  _receiveState = ReceiveState::ReadPreview;
  memset(_preview, 0, sizeof(_preview));
}

bool TcpConnectionManager::isKnownControlByte(uint8_t control) { return control == 0x09 || control == 0x0A; }

uint32_t TcpConnectionManager::decodeBigEndian32(const uint8_t* data) {
  return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) | (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
}

uint32_t TcpConnectionManager::getTotalLength(const uint8_t* preview) {
  switch (preview[0]) {
    case 0x09: return decodeBigEndian32(&preview[3]);
    case 0x0A: return decodeBigEndian32(&preview[1]);
    default: return 0;
  }
}

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "TcpMessageQueue.h"

class TcpConnectionManager {
public:
    TcpConnectionManager(uint16_t port, TcpMessageQueue& receiveQueue, TcpMessageQueue& transmitQueue);
    bool begin(const char* ssid, const char* password, BaseType_t core = 1, UBaseType_t priority = 1);
    bool connected();
 
private:
    enum class ReceiveState : uint8_t { ReadPreview, ReadMessage };
    static void taskEntry(void* parameter);
    void task();
    void acceptClient();
    void handleDisconnect();
    void processIncomingData();
    void processOutgoingData();
    void readPreview();
    void prepareMessage();
    void readMessageBody();
    void finishMessage();
    bool sendCompleteMessage(const TcpMessage& message);
    void resetReceiver();
    bool isKnownControlByte(uint8_t control);
    uint32_t decodeBigEndian32(const uint8_t* data);
    uint32_t getTotalLength(const uint8_t* preview);

    WiFiServer _server;
    WiFiClient _client;
    TcpMessageQueue& _receiveQueue;
    TcpMessageQueue& _transmitQueue;
    TaskHandle_t _taskHandle;
    ReceiveState _receiveState;

    static constexpr size_t PREVIEW_SIZE = 7;
    uint8_t _preview[PREVIEW_SIZE];
    size_t _previewReceived;
    uint8_t* _messageBuffer;
    uint32_t _messageLength;
    uint32_t _messageReceived;

    static constexpr uint32_t MAX_MESSAGE_SIZE = 327680UL + 14UL;
    static constexpr size_t NETWORK_CHUNK_SIZE = 4096;
};

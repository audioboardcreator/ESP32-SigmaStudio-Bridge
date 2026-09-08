#pragma once
#include <Arduino.h>

struct TcpMessage {
    uint32_t length;
    uint8_t* data;
};
 
class TcpMessageQueue {
public:
    explicit TcpMessageQueue(UBaseType_t queueLength = 2);
    bool begin();
    bool send(const TcpMessage& message, TickType_t timeout = 0);
    bool receive(TcpMessage& message, TickType_t timeout = portMAX_DELAY);
    void release(TcpMessage& message);
    bool isReady() const;

private:
    QueueHandle_t _queue;
    UBaseType_t _queueLength;
};

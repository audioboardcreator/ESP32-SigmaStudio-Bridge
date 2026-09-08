#include "TcpMessageQueue.h"
#include <cstdlib>
#include "DualConsole.h"

TcpMessageQueue::TcpMessageQueue(UBaseType_t queueLength) : _queue(nullptr), _queueLength(queueLength) {}

bool TcpMessageQueue::begin() {
    if (_queue != nullptr) return true;
    if (_queueLength == 0) return false;
    _queue = xQueueCreate(_queueLength, sizeof(TcpMessage));
    return (_queue != nullptr);
}

bool TcpMessageQueue::send(const TcpMessage& message, TickType_t timeout) {
    if (_queue == nullptr) return false;
    if (message.data == nullptr || message.length == 0) return false;
    return xQueueSend(_queue, &message, timeout) == pdTRUE;
}

bool TcpMessageQueue::receive(TcpMessage& message, TickType_t timeout) {
    message.length = 0; message.data = nullptr;
    if (_queue == nullptr) return false;
    return xQueueReceive(_queue, &message, timeout) == pdTRUE;
}

void TcpMessageQueue::release(TcpMessage& message) {
    if (message.data != nullptr) { free(message.data); message.data = nullptr; }
    message.length = 0;
}

bool TcpMessageQueue::isReady() const { return _queue != nullptr; }

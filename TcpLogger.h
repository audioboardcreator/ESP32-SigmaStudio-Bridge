#pragma once
#include <Arduino.h>

enum class LogType : uint8_t { RX, TX };

struct LogMessage {
    uint32_t timestamp;
    LogType type;
    uint32_t length;
    uint8_t* data;
};

class TcpLogger {
public:
    TcpLogger();
    bool begin(BaseType_t core = 1, UBaseType_t priority = 1);
    void log(LogType type, const uint8_t* data, uint32_t length);
    static bool active;

private:
    static void loggerTaskEntry(void* parameter);
    void loggerTask();
    void parseAndPrintMessage(const LogMessage& msg);

    QueueHandle_t _logQueue;
    TaskHandle_t _taskHandle;
    static constexpr UBaseType_t LOG_QUEUE_LENGTH = 20;
};

extern TcpLogger tcpLogger;

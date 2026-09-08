#include "TcpLogger.h"
#include "esp_heap_caps.h"
#include "DualConsole.h"

bool TcpLogger::active = false; 
TcpLogger tcpLogger;

TcpLogger::TcpLogger() : _logQueue(nullptr), _taskHandle(nullptr) {}

bool TcpLogger::begin(BaseType_t core, UBaseType_t priority) {
    if (_taskHandle != nullptr) return true;
    _logQueue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogMessage));
    if (_logQueue == nullptr) return false;
    BaseType_t result = xTaskCreatePinnedToCore(loggerTaskEntry, "TcpLoggerTask", 8192, this, priority, &_taskHandle, core);
    return (result == pdPASS);
}

void TcpLogger::log(LogType type, const uint8_t* data, uint32_t length) {
    if (!active || data == nullptr || length == 0) return;
    uint8_t* bufferCopy = static_cast<uint8_t*>(heap_caps_malloc(length, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (bufferCopy == nullptr) return;
    memcpy(bufferCopy, data, length);
    LogMessage msg{ millis(), type, length, bufferCopy };
    if (xQueueSend(_logQueue, &msg, 0) != pdTRUE) { free(bufferCopy); }
}

void TcpLogger::loggerTaskEntry(void* parameter) {
    static_cast<TcpLogger*>(parameter)->loggerTask();
}

void TcpLogger::loggerTask() {
    LogMessage msg;
    while (true) {
        if (xQueueReceive(_logQueue, &msg, portMAX_DELAY) == pdTRUE) {
            parseAndPrintMessage(msg);
            if (msg.data != nullptr) { free(msg.data); }
        }
    }
}

uint16_t readBE16(const uint8_t* d) { return (d[0] << 8) | d[1]; }
uint32_t readBE32(const uint8_t* d) { return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3]; }

void TcpLogger::parseAndPrintMessage(const LogMessage& msg) {
    Serial.println("\n========================================================");
    Serial.printf("[%lu ms] Direction: %s\n", msg.timestamp, (msg.type == LogType::RX) ? "INCOMING (FROM PC)" : "OUTGOING (TO PC)");
    Serial.printf("Raw Data Length: %lu Bytes\n", msg.length);
    Serial.println("--------------------------------------------------------");
    
    // HIER DER FIX: Index [0] auslesen
    uint8_t control = msg.data[0]; 
    
    if (control == 0x09 && msg.length >= 14) {
        uint32_t totalLength = readBE32(&msg.data[3]);
        uint8_t chipAddr     = msg.data[7];
        uint32_t dataLength  = readBE32(&msg.data[8]);
        uint16_t address     = readBE16(&msg.data[12]);
        uint8_t safeload     = msg.data[1];
        Serial.printf("Command:         0x09 [WRITE REQUEST]\n");
        Serial.printf("Type:            %s\n", (safeload == 1) ? "Safeload Write" : "Block Write");
        Serial.printf("Chip Address:    0x%02X\n", chipAddr);
        Serial.printf("DSP Target Addr: 0x%04X\n", address);
        Serial.printf("Payload Length:  %lu Bytes (Total in header: %lu)\n", dataLength, totalLength);
        Serial.print("Payload (Hex):   ");
        uint32_t dumpLen = (dataLength > 16) ? 16 : dataLength;
        for(uint32_t i=0; i<dumpLen; i++) Serial.printf("%02X ", msg.data[14 + i]);
        if (dataLength > 16) Serial.print("... [truncated]");
        Serial.println();
    }
    else if (control == 0x0A && msg.length >= 14) {
        uint32_t totalLength = readBE32(&msg.data[1]);
        uint8_t chipAddr     = msg.data[5];
        uint32_t dataLength  = readBE32(&msg.data[6]);
        uint16_t address     = readBE16(&msg.data[10]);
        Serial.printf("Command:         0x0A [READ REQUEST]\n");
        Serial.printf("Chip Address:    0x%02X\n", chipAddr);
        Serial.printf("DSP Source Addr: 0x%04X\n", address);
        Serial.printf("Requested Size:  %lu Bytes to read (Total: %lu)\n", dataLength, totalLength);
    }
    else if (control == 0x0B && msg.length >= 14) {
        uint32_t totalLength = readBE32(&msg.data[1]);
        uint8_t chipAddr     = msg.data[5];
        uint32_t dataLength  = readBE32(&msg.data[6]);
        uint16_t address     = readBE16(&msg.data[10]);
        uint8_t status       = msg.data[12];
        Serial.printf("Command:         0x0B [READ RESPONSE]\n");
        Serial.printf("Status:          %s\n", (status == 0x00) ? "SUCCESS (0x00)" : "FAILURE (0x01)");
        Serial.printf("Chip Address:    0x%02X\n", chipAddr);
        Serial.printf("DSP Source Addr: 0x%04X\n", address);
        Serial.printf("Data Length:     %lu Bytes\n", dataLength);
        Serial.print("Data (Hex):      ");
        uint32_t dumpLen = (dataLength > 16) ? 16 : dataLength;
        for(uint32_t i=0; i<dumpLen; i++) Serial.printf("%02X ", msg.data[14 + i]);
        if (dataLength > 16) Serial.print("... [truncated]");
        Serial.println();
    }
    else {
        Serial.printf("Command:         0x%02X [UNKNOWN OR FRAGMENT]\n", control);
        Serial.print("Raw Data (Hex):  ");
        uint32_t dumpLen = (msg.length > 32) ? 32 : msg.length;
        for(uint32_t i=0; i<dumpLen; i++) Serial.printf("%02X ", msg.data[i]);
        if (msg.length > 32) Serial.print("... [truncated]");
        Serial.println();
    }
    Serial.println("========================================================");
}

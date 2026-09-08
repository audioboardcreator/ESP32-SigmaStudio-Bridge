#pragma once
#include <functional>
#include <Arduino.h>
#include <LittleFS.h>

class TcpRecorder {
public:
    TcpRecorder();
    bool begin();
    bool startRecording(const String& fileName);
    void stopRecording();
    void recordSpiWrite(uint16_t address, const uint8_t* data, uint32_t length);
    bool playMacro(const String& fileName, std::function<bool(uint16_t, const uint8_t*, uint32_t)> writeSpiCallback);
    bool deleteMacro(const String& fileName);
    void listFiles();
    String getFileNameByIndex(uint32_t targetIndex);
    static bool isRecording;

private:
    File _recordFile;
    String _currentFileName;
};

extern TcpRecorder tcpRecorder;
 
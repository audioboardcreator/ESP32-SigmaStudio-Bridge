#pragma once
#include <Arduino.h>
#include <WiFi.h>
 
class DualConsole {
public:
    DualConsole(HardwareSerial& serialHardware);
    void begin(unsigned long baud);
    void setNetworkClient(WiFiClient* client);
    int available();
    int read();
    String readStringUntil(char terminator);

    template <typename T>
    void print(T data) {
        _serial.print(data);
        if (_networkClient && _networkClient->connected()) _networkClient->print(data);
    }

    template <typename T>
    void println(T data) {
        _serial.println(data);
        if (_networkClient && _networkClient->connected()) _networkClient->println(data);
    }

    void println() {
        _serial.println();
        if (_networkClient && _networkClient->connected()) _networkClient->println();
    }

    void printf(const char* format, ...);

private:
    HardwareSerial& _serial;
    WiFiClient* _networkClient;
    String _inputBuffer;
};

extern DualConsole Console;
#define Serial Console

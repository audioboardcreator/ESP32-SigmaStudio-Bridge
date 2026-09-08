#include "DualConsole.h"
#include <cstdarg>
#include <cstdio>
#include <string>

DualConsole Console(Serial0);

DualConsole::DualConsole(HardwareSerial& serialHardware) 
    : _serial(serialHardware), _networkClient(nullptr), _inputBuffer("") {}

void DualConsole::begin(unsigned long baud) {
    _serial.begin(baud);
}

void DualConsole::setNetworkClient(WiFiClient* client) {
    _networkClient = client;
}

int DualConsole::available() {
    if (_inputBuffer.indexOf('\n') != -1 || _inputBuffer.indexOf('\r') != -1) return 1;
    int serialAvailable = _serial.available();
    int networkAvailable = (_networkClient && _networkClient->connected()) ? _networkClient->available() : 0;
    return serialAvailable + networkAvailable;
}

int DualConsole::read() {
    if (_networkClient && _networkClient->connected() && _networkClient->available() > 0) {
        return _networkClient->read();
    }
    return _serial.read();
}

String DualConsole::readStringUntil(char terminator) {
    while (_serial.available() > 0 || (_networkClient && _networkClient->connected() && _networkClient->available() > 0)) {
        char c;
        if (_networkClient && _networkClient->connected() && _networkClient->available() > 0) {
            c = static_cast<char>(_networkClient->read());
        } else {
            c = static_cast<char>(_serial.read());
            _serial.write(c); 
        }

        if (c == terminator || c == '\r') {
            String completedLine = _inputBuffer;
            _inputBuffer = ""; 
            _serial.println();
            if (_networkClient && _networkClient->connected()) _networkClient->println();
            return completedLine;
        }
        _inputBuffer += c;
    }
    return "";
}

void DualConsole::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    va_list argsCopy;
    va_copy(argsCopy, args);
    int len = vsnprintf(nullptr, 0, format, argsCopy);
    va_end(argsCopy);

    if (len < 0) { va_start(args, format); va_end(args); return; }

    std::string text(len, '\0');
    vsnprintf(text.data(), len + 1, format, args);
    va_end(args);

    _serial.write(reinterpret_cast<const uint8_t*>(text.c_str()), text.length());

    if (_networkClient && _networkClient->connected()) {
        for (char c : text) {
            if (c == '\n') _networkClient->write('\r');
            _networkClient->write(c);
        }
    }
}

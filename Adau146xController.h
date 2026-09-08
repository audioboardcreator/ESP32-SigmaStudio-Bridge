#pragma once
#include <WiFi.h>
#include "Config.h"
#include "TcpMessageQueue.h"
#include "TcpConnectionManager.h"
#include "Adau1467.h"

class Adau146XController {
public:
    Adau146XController();
    bool begin();
    void run();
    Adau1467& getDsp();
 
private:
    void handleAutoboot();
    void handleConsoleConnect();
    void handleConsoleDisconnect();
    void processConsole();
    void processCommand(const String& input);

    TcpMessageQueue _receiveQueue;
    TcpMessageQueue _transmitQueue;
    TcpConnectionManager _tcpServer;
    Adau1467 _adau1467;
    WiFiServer _consoleServer;
    WiFiClient _consoleClient;
};

extern Adau146XController controller;

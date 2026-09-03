#include "Config.h"
#include "TcpMessageQueue.h"
#include "TcpConnectionManager.h"
#include "Adau1467.h"

TcpMessageQueue receiveQueue(
    RX_QUEUE_LENGTH);

TcpMessageQueue transmitQueue(
    TX_QUEUE_LENGTH);

TcpConnectionManager tcpServer(
    SERVER_PORT,
    receiveQueue,
    transmitQueue);

Adau1467 adau1467(
    receiveQueue,
    transmitQueue);

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(
        "Systemstart");

    if (!receiveQueue.begin())
    {
        Serial.println(
            "RX-Queue konnte nicht initialisiert werden");

        return;
    }

    if (!transmitQueue.begin())
    {
        Serial.println(
            "TX-Queue konnte nicht initialisiert werden");

        return;
    }

    if (!adau1467.begin(
            ADAU_SPI_HOST,
            PIN_MOSI,
            PIN_MISO,
            PIN_SCLK,
            PIN_CS,
            ADAU_SPI_CLOCK_HZ,
            ADAU_TASK_CORE,
            ADAU_TASK_PRIORITY))
    {
        Serial.println(
            "ADAU1467-Initialisierung fehlgeschlagen");

        return;
    }

    if (!tcpServer.begin(
            WIFI_SSID,
            WIFI_PASSWORD,
            TCP_TASK_CORE,
            TCP_TASK_PRIORITY))
    {
        Serial.println(
            "TCP-Initialisierung fehlgeschlagen");

        return;
    }

    Serial.println(
        "System vollstaendig gestartet");
}

void loop()
{
    delay(1000);
}
#include "Config.h"
#include "TcpMessageQueue.h"
#include "TcpConnectionManager.h"
#include "Adau1467.h"
#include "TcpLogger.h"
#include "TcpRecorder.h"

TcpMessageQueue receiveQueue(RX_QUEUE_LENGTH);
TcpMessageQueue transmitQueue(TX_QUEUE_LENGTH);
TcpConnectionManager tcpServer(SERVER_PORT, receiveQueue, transmitQueue);
Adau1467 adau1467(receiveQueue, transmitQueue);

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("System starting...");

    if (!receiveQueue.begin()) { Serial.println("Error: Failed to initialize RX queue"); return; }
    if (!transmitQueue.begin()) { Serial.println("Error: Failed to initialize TX queue"); return; }
    if (!adau1467.begin(ADAU_SPI_HOST, PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS, ADAU_SPI_CLOCK_HZ, ADAU_TASK_CORE, ADAU_TASK_PRIORITY)) {
        Serial.println("Error: ADAU1467 initialization failed");
        return;
    }
    if (!tcpRecorder.begin()) { Serial.println("Error: Failed to initialize SPI recorder (LittleFS)"); }
    if (!tcpLogger.begin(1, 1)) { Serial.println("Error: Failed to start background TCP logger"); }
    if (!tcpServer.begin(WIFI_SSID, WIFI_PASSWORD, TCP_TASK_CORE, TCP_TASK_PRIORITY)) {
        Serial.println("Error: TCP bridge server initialization failed");
        return;
    }
    Serial.println("System fully booted and ready");
    // -----------------------------------------------------------------
    // AUTOBREAK / AUTOBOOT LOGIC
    // -----------------------------------------------------------------
    // We check for a specific default file named "boot_prog"
    if (LittleFS.exists("/boot_prog.bin")) {
        Serial.println("\n[AUTOBOOT] Found default firmware configuration ('boot_prog')!");
        Serial.println("[AUTOBOOT] Initializing autonomous DSP flashing sequence...");
        
        auto spiCallback = [](uint16_t addr, const uint8_t* data, uint32_t len) -> bool {
            return adau1467.writeSpi(addr, data, len);
        };
        
        if (tcpRecorder.playMacro("boot_prog", spiCallback)) {
            Serial.println("[AUTOBOOT] DSP successfully configured autonomously.\n");
        } else {
            Serial.println("[AUTOBOOT] Warning: Autonomous DSP flashing encountered errors.\n");
        }
    } else {
        Serial.println("\n[AUTOBOOT] No 'boot_prog.bin' found. Standing by for SigmaStudio or CLI...\n");
    }
    
    tcpRecorder.listFiles();
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        int spaceIndex = input.indexOf(' ');
        String command = (spaceIndex != -1) ? input.substring(0, spaceIndex) : input;
        String argument = (spaceIndex != -1) ? input.substring(spaceIndex + 1) : "";
        command.trim(); argument.trim();

        if (command.equalsIgnoreCase("list")) {
            tcpRecorder.listFiles();
        }
        else if (command.equalsIgnoreCase("rec_on")) {
            String name = (argument.length() > 0) ? argument : "macro";
            tcpRecorder.startRecording(name);
        }
        else if (command.equalsIgnoreCase("rec_off")) {
            tcpRecorder.stopRecording();
            tcpRecorder.listFiles();
        }
        else if (command.equalsIgnoreCase("replay")) {
            String targetName = argument;
            if (targetName.length() == 0) {
                Serial.println("[RECORDER] Error: Please specify a number or file name (e.g., 'replay 1')");
            } else {
                bool isNumber = true;
                for (unsigned int i = 0; i < targetName.length(); i++) {
                    if (!isDigit(targetName[i])) { isNumber = false; break; }
                }
                if (isNumber) {
                    uint32_t index = targetName.toInt();
                    String resolvedName = tcpRecorder.getFileNameByIndex(index);
                    if (resolvedName.length() > 0) { targetName = resolvedName; } 
                    else { Serial.printf("[RECORDER] Error: Macro index [%lu] does not exist.\n", index); targetName = ""; }
                }
                if (targetName.length() > 0) {
                    auto spiCallback = [](uint16_t addr, const uint8_t* data, uint32_t len) -> bool {
                        return adau1467.writeSpi(addr, data, len);
                    };
                    tcpRecorder.playMacro(targetName, spiCallback);
                }
            }
        }
        else if (command.equalsIgnoreCase("tcp_log_on")) {
            TcpLogger::active = true;
            Serial.println("[LOG] Live packet logging ENABLED.");
        }
        else if (command.equalsIgnoreCase("tcp_log_off")) {
            TcpLogger::active = false;
            Serial.println("[LOG] Live packet logging DISABLED.");
        }
        else if (command.equalsIgnoreCase("status")) {
            Serial.println("\n--- SYSTEM STATUS ---");
            Serial.printf("  Network Logger:  %s\n", TcpLogger::active ? "ENABLED" : "DISABLED");
            Serial.printf("  Hardware Capture:%s\n", TcpRecorder::isRecording ? "RECORDING IN PROGRESS" : "READY / IDLE");
            Serial.println("---------------------\n");
        }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

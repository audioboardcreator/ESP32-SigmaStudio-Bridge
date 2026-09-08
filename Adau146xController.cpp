#include "Adau146xController.h"
#include <LittleFS.h>
#include "DualConsole.h"
#include "TcpLogger.h"
#include "TcpRecorder.h"

Adau146XController controller;

Adau146XController::Adau146XController()
  : _receiveQueue(RX_QUEUE_LENGTH), _transmitQueue(TX_QUEUE_LENGTH), _tcpServer(SERVER_PORT, _receiveQueue, _transmitQueue), _adau1467(_receiveQueue, _transmitQueue), _consoleServer(8087) {
}

bool Adau146XController::begin() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("System starting...");
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  delay(100);
  Serial.println("[SYSTEM] Powering on FSC-BT1026D Bluetooth...");
  digitalWrite(4, HIGH);
  delay(3000);
  digitalWrite(4, LOW);
  Serial.println("[SYSTEM] Bluetooth power pulse completed.");

  if (!_receiveQueue.begin()) {
    Serial.println("Error: Failed to initialize RX queue");
    return false;
  }
  if (!_transmitQueue.begin()) {
    Serial.println("Error: Failed to initialize TX queue");
    return false;
  }
  if (!_adau1467.begin(ADAU_SPI_HOST, PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS, ADAU_SPI_CLOCK_HZ, ADAU_TASK_CORE, ADAU_TASK_PRIORITY)) {
    Serial.println("Error: _adau1467 initialization failed");
    return false;
  }
  if (!tcpRecorder.begin()) {
    Serial.println("Error: Failed to initialize SPI recorder (LittleFS)");
  }
  if (!tcpLogger.begin(1, 1)) {
    Serial.println("Error: Failed to start background TCP logger");
  }
  if (!_tcpServer.begin(WIFI_SSID, WIFI_PASSWORD, TCP_TASK_CORE, TCP_TASK_PRIORITY)) {
    Serial.println("Error: TCP bridge server initialization failed");
    return false;
  }

  _consoleServer.begin();
  Serial.println("System fully booted and ready. Wireless console listening on port 8087.");
  handleAutoboot();
  tcpRecorder.listFiles();
  return true;
}

void Adau146XController::run() {
  handleConsoleDisconnect();
  handleConsoleConnect();
  processConsole();
  vTaskDelay(pdMS_TO_TICKS(50));
  neopixelWrite(PIN_ONBOARD_LED, 0, 0, 0);
}

void Adau146XController::handleConsoleConnect() {
  if (!_consoleServer.hasClient()) return;
  if (_consoleClient) _consoleClient.stop();

  _consoleClient = _consoleServer.available();
  Console.setNetworkClient(&_consoleClient);
  Serial.println();
  Serial.println("=== Connected to ESP32 Wireless Console ===");
  tcpRecorder.listFiles();
}

void Adau146XController::handleConsoleDisconnect() {
  if (!_consoleClient) return;
  if (_consoleClient.connected()) return;

  _consoleClient.stop();
  Console.setNetworkClient(nullptr);
}

void Adau146XController::processConsole() {
  if (Serial.available() <= 0) return;
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;
  processCommand(input);
}

void Adau146XController::processCommand(const String& input) {
  int spaceIndex = input.indexOf(' ');
  String command = (spaceIndex != -1) ? input.substring(0, spaceIndex) : input;
  String argument = (spaceIndex != -1) ? input.substring(spaceIndex + 1) : "";
  command.trim();
  argument.trim();

  if (command.equalsIgnoreCase("list")) {
    tcpRecorder.listFiles();
  } else if (command.equalsIgnoreCase("rec_on")) {
    String name = (argument.length() > 0) ? argument : "macro";
    tcpRecorder.startRecording(name);
  } else if (command.equalsIgnoreCase("rec_off")) {
    tcpRecorder.stopRecording();
    tcpRecorder.listFiles();
  } else if (command.equalsIgnoreCase("replay")) {
    String targetName = argument;
    if (targetName.length() == 0) {
      Serial.println("[RECORDER] Error: Please specify a number or file name (e.g., 'replay 1')");
    } else {
      bool isNumber = true;
      for (unsigned int i = 0; i < targetName.length(); i++) {
        if (!isDigit(targetName[i])) {
          isNumber = false;
          break;
        }
      }
      if (isNumber) {
        uint32_t index = targetName.toInt();
        String resolvedName = tcpRecorder.getFileNameByIndex(index);
        if (resolvedName.length() > 0) {
          targetName = resolvedName;
        } else {
          Serial.printf("[RECORDER] Error: Macro index [%lu] does not exist.\n", (unsigned long)index);
          targetName = "";
        }
      }
      if (targetName.length() > 0) {
        tcpRecorder.playMacro(targetName, [this](uint16_t addr, const uint8_t* data, uint32_t len) {
          return _adau1467.writeSpi(addr, data, len);
        });
      }
    }
  } else if (command.equalsIgnoreCase("tcp_log_on")) {
    TcpLogger::active = true;
    Serial.println("[LOG] Live packet logging ENABLED.");
  } else if (command.equalsIgnoreCase("tcp_log_off")) {
    TcpLogger::active = false;
    Serial.println("[LOG] Live packet logging DISABLED.");
  } else if (command.equalsIgnoreCase("status")) {
    Serial.println("\n--- SYSTEM STATUS ---");
    Serial.printf("  Network Logger:  %s\n", TcpLogger::active ? "ENABLED" : "DISABLED");
    Serial.printf("  Hardware Capture:%s\n", TcpRecorder::isRecording ? "RECORDING IN PROGRESS" : "READY / IDLE");
    Serial.println("---------------------\n");
  } else if (command.equalsIgnoreCase("exit") || command.equalsIgnoreCase("quit") || command.equalsIgnoreCase("logout")) {
    Serial.println("[CONSOLE] Disconnecting client...");
    if (_consoleClient.connected()) _consoleClient.stop();
    Console.setNetworkClient(nullptr);
  } else if (command.equalsIgnoreCase("set_reg")) {
    int firstSpace = argument.indexOf(' ');
    if (firstSpace < 0) {
      Serial.println("Usage: set_reg <address> <byte1> [byte2] [byte3] ...");
      return;
    }
    String addressStr = argument.substring(0, firstSpace);
    String dataStr = argument.substring(firstSpace + 1);
    uint16_t address = (uint16_t)strtoul(addressStr.c_str(), nullptr, 0);
    uint8_t data[256];
    uint32_t length = 0;

    while (dataStr.length() > 0 && length < sizeof(data)) {
      int nextSpace = dataStr.indexOf(' ');
      String token;
      if (nextSpace >= 0) {
        token = dataStr.substring(0, nextSpace);
        dataStr = dataStr.substring(nextSpace + 1);
        dataStr.trim();
      } else {
        token = dataStr;
        dataStr = "";
      }
      data[length++] = (uint8_t)strtoul(token.c_str(), nullptr, 0);
    }

    if (length == 0) {
      Serial.println("Error: No data bytes specified.");
    } else if (_adau1467.writeSpi(address, data, length)) {
      Serial.printf("[SPI] Wrote %lu byte(s) to 0x%04X\n", (unsigned long)length, address);
    } else {
      Serial.println("[SPI] Write failed.");
    }
  } else if (command.equalsIgnoreCase("get_reg")) {
    int firstSpace = argument.indexOf(' ');
    if (firstSpace < 0) {
      Serial.println("Usage: get_reg <address> <length>");
      return;
    }
    String addressStr = argument.substring(0, firstSpace);
    String lengthStr = argument.substring(firstSpace + 1);
    uint16_t address = (uint16_t)strtoul(addressStr.c_str(), nullptr, 0);
    uint32_t length = strtoul(lengthStr.c_str(), nullptr, 0);

    if (length == 0 || length > 256) {
      Serial.println("Error: Length must be between 1 and 256.");
      return;
    }
    uint8_t data[256];

    if (_adau1467.readSpi(address, data, length)) {
      Serial.printf("[SPI] Read %lu byte(s) from 0x%04X:\n", (unsigned long)length, address);
      for (uint32_t i = 0; i < length; i++) {
        Serial.printf("0x%02X ", data[i]);
      }
      Serial.println();
    } else {
      Serial.println("[SPI] Read failed.");
    }
  } else if (command.equalsIgnoreCase("rec_remove")) {
    String targetName = argument;
    if (targetName.length() == 0) {
      Serial.println("[RECORDER] Error: Please specify a macro name.");
    } else {
      tcpRecorder.deleteMacro(targetName);
    }
  }
}
 
void Adau146XController::handleAutoboot() {
  if (!LittleFS.exists("/boot_prog.bin")) {
    Serial.println("\n[AUTOBOOT] No 'boot_prog.bin' found. Standing by for SigmaStudio or CLI...\n");
    return;
  }
  Serial.println("\n[AUTOBOOT] Found default firmware configuration ('boot_prog')!");
  Serial.println("[AUTOBOOT] Initializing autonomous DSP flashing sequence...");

  if (tcpRecorder.playMacro("boot_prog", [this](uint16_t addr, const uint8_t* data, uint32_t len) {
    return _adau1467.writeSpi(addr, data, len);
  })) {
    Serial.println("[AUTOBOOT] DSP successfully configured autonomously.\n");
  } else {
    Serial.println("[AUTOBOOT] Warning: Autonomous DSP flashing encountered errors.\n");
  }
}

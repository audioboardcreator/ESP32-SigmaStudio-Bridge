#include "TcpRecorder.h"
#include "DualConsole.h"

bool TcpRecorder::isRecording = false;
TcpRecorder tcpRecorder;

TcpRecorder::TcpRecorder() : _currentFileName("") {}

bool TcpRecorder::begin() {
  if (!LittleFS.begin(true)) {
    Serial.println("[RECORDER] Error: LittleFS initialization failed!");
    return false;
  }
  return true;
}

bool TcpRecorder::startRecording(const String& fileName) {
    if (isRecording) {
        Serial.println("[RECORDER] Error: Recording is already in progress!");
        return false;
    }
    _currentFileName = "/" + fileName + ".bin";
    if (LittleFS.exists(_currentFileName)) {
        LittleFS.remove(_currentFileName);
    }
    _recordFile = LittleFS.open(_currentFileName, "w");
    if (!_recordFile) {
        Serial.printf("[RECORDER] Error: Failed to create file %s!\n", _currentFileName.c_str());
        return false;
    }
    isRecording = true;
    Serial.printf("[RECORDER] Recording started: %s\n", _currentFileName.c_str());
    return true;
}

void TcpRecorder::stopRecording() {
  if (!isRecording) return;
  isRecording = false;
  if (_recordFile) {
    _recordFile.close();
  }
  Serial.printf("[RECORDER] Recording stopped. File saved as: %s\n", _currentFileName.c_str());
}

void TcpRecorder::recordSpiWrite(uint16_t address, const uint8_t* data, uint32_t length) {
  if (!isRecording || !_recordFile || data == nullptr || length == 0) return;
  uint8_t addrBuf[2];
  addrBuf[0] = static_cast<uint8_t>(address >> 8);
  addrBuf[1] = static_cast<uint8_t>(address & 0xFF);
  _recordFile.write(addrBuf, 2);
  _recordFile.write(reinterpret_cast<const uint8_t*>(&length), 4);
  _recordFile.write(data, length);
  _recordFile.flush();
}

bool TcpRecorder::playMacro(const String& fileName, std::function<bool(uint16_t, const uint8_t*, uint32_t)> writeSpiCallback) {
  if (isRecording) {
    Serial.println("[RECORDER] Error: Cannot start playback while recording!");
    return false;
  }
  String path = "/" + fileName + ".bin";
  File file = LittleFS.open(path, FILE_READ);
  if (!file) {
    Serial.printf("[RECORDER] Error: File %s not found!\n", path.c_str());
    return false;
  }
  Serial.printf("[RECORDER] Starting standalone replay of: %s\n", path.c_str());
  uint32_t commandCount = 0;
  while (file.available() >= 6) {
    uint8_t addrBuf[2];
    file.read(addrBuf, 2);
    uint16_t address = (addrBuf[0] << 8) | addrBuf[1];
    uint32_t length = 0;
    file.read(reinterpret_cast<uint8_t*>(&length), 4);
    uint8_t* dataBuffer = static_cast<uint8_t*>(malloc(length));
    if (dataBuffer == nullptr) {
      Serial.println("[RECORDER] Critical Error: Out of memory for macro payload!");
      file.close();
      return false;
    }
    file.read(dataBuffer, length);
    if (writeSpiCallback != nullptr) {
      writeSpiCallback(address, dataBuffer, length);
    }
    free(dataBuffer);
    commandCount++;
  }
  file.close();
  Serial.printf("[RECORDER] Playback finished. %lu SPI commands successfully transmitted.\n", commandCount);
  return true;
}

void TcpRecorder::listFiles() {
  Serial.println("\n=== COMMANDS & CONTROLS ===");
  Serial.println("  rec_on [Name]     : Starts the SPI hardware capture (Default name: macro)");
  Serial.println("  rec_off           : Stops current recording and saves to internal storage");
  Serial.println("  rec_remove [Name] : Deletes the specified macro");  
  Serial.println("  replay [Nr]       : Replays the specific macro by its listed index number");
  Serial.println("  replay [Name]     : Alternative: Replays the macro directly by file name");
  Serial.println("  tcp_log_on        : Enables live TCP traffic monitoring");
  Serial.println("  tcp_log_off       : Disables live TCP traffic monitoring");
  Serial.println("  status            : Shows the system status and current recording state");
  Serial.println("  list              : Refreshes this overview and updates the file list");

  Serial.println("\n=== AVAILABLE DSP MACROS (LITTLEFS) ===");
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) { Serial.println("Error: Failed to read local file system."); return; }
  File file = root.openNextFile();
  uint32_t index = 1;
  bool foundFiles = false;
  while (file) {
    String name = file.name();
    if (name.endsWith(".bin")) {
      if (name.startsWith("/")) { name = name.substring(1); }
      if (name.endsWith(".bin")) { name = name.substring(0, name.length() - 4); }
      Serial.printf("  [%lu] %s (%lu Bytes)\n", index, name.c_str(), file.size());
      index++;
      foundFiles = true;
    }
    file = root.openNextFile();
  }
  if (!foundFiles) { Serial.println("  (No recorded standalone macros found)"); }
  Serial.println("==================================================\n");
}

String TcpRecorder::getFileNameByIndex(uint32_t targetIndex) {
  if (targetIndex == 0) return "";
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) return "";
  File file = root.openNextFile();
  uint32_t currentIndex = 1;
  while (file) {
    String name = file.name();
    if (name.endsWith(".bin")) {
      if (currentIndex == targetIndex) {
        if (name.startsWith("/")) { name = name.substring(1); }
        if (name.endsWith(".bin")) { name = name.substring(0, name.length() - 4); }
        file.close();
        return name;
      }
      currentIndex++;
    }
    file = root.openNextFile();
  }
  return "";
}

bool TcpRecorder::deleteMacro(const String& fileName) {
    if (isRecording) { Serial.println("[RECORDER] Error: Cannot delete while recording!"); return false; }
    String path = "/" + fileName + ".bin";
    if (!LittleFS.exists(path)) { Serial.printf("[RECORDER] Error: File %s not found!\n", path.c_str()); return false; }
    if (LittleFS.remove(path)) { Serial.printf("[RECORDER] Deleted: %s\n", path.c_str()); return true; }
    Serial.printf("[RECORDER] Failed to delete: %s\n", path.c_str());
    return false;
}

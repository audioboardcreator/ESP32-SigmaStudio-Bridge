#include "TcpRecorder.h"

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
    _recordFile = LittleFS.open(_currentFileName, FILE_WRITE);
    
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

    // 1. Write address (Big Endian for consistency)
    uint8_t addrBuf[2];
    addrBuf[0] = static_cast<uint8_t>(address >> 8);
    addrBuf[1] = static_cast<uint8_t>(address & 0xFF);
    _recordFile.write(addrBuf, 2);

    // 2. Write data length (4 Bytes)
    _recordFile.write(reinterpret_cast<const uint8_t*>(&length), 4);

    // 3. Write raw data bytes
    _recordFile.write(data, length);
    
    // Force data write to flash
    _recordFile.flush();
}

bool TcpRecorder::playMacro(const String& fileName, bool (*writeSpiCallback)(uint16_t, const uint8_t*, uint32_t)) {
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

    while (file.available() >= 6) { // 2 Bytes address + 4 Bytes length = at least 6 bytes
        // 1. Read address
        uint8_t addrBuf[2];
        file.read(addrBuf, 2);
        uint16_t address = (addrBuf[0] << 8) | addrBuf[1];

        // 2. Read length
        uint32_t length = 0;
        file.read(reinterpret_cast<uint8_t*>(&length), 4);

        // 3. Allocate data buffer and read payload
        uint8_t* dataBuffer = static_cast<uint8_t*>(malloc(length));
        if (dataBuffer == nullptr) {
            Serial.println("[RECORDER] Critical Error: Out of memory for macro payload!");
            file.close();
            return false;
        }

        file.read(dataBuffer, length);

        // 4. Transmit directly to hardware via callback
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
    Serial.println("  rec_on [Name] : Starts the SPI hardware capture (Default name: macro)");
    Serial.println("  rec_off       : Stops current recording and saves to internal storage");
    Serial.println("  replay [Nr]   : Replays the specific macro by its listed index number");
    Serial.println("  replay [Name] : Alternative: Replays the macro directly by file name");
    Serial.println("  status        : Shows the system status and current recording state");
    Serial.println("  list          : Refreshes this overview and updates the file list");
    
    Serial.println("\n=== AVAILABLE DSP MACROS (LITTLEFS) ===");
    
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("Error: Failed to read local file system.");
        return;
    }

    File file = root.openNextFile();
    uint32_t index = 1;
    bool foundFiles = false;

    while (file) {
        String name = file.name();
        if (name.endsWith(".bin")) {
            // Remove leading slash for cleaner display
            if (name.startsWith("/")) { name = name.substring(1); }
            // Strip ".bin" extension for the menu list
            if (name.endsWith(".bin")) { name = name.substring(0, name.length() - 4); }
            
            Serial.printf("  [%lu] %s (%lu Bytes)\n", index, name.c_str(), file.size());
            index++;
            foundFiles = true;
        }
        file = root.openNextFile();
    }

    if (!foundFiles) {
        Serial.println("  (No recorded standalone macros found)");
    }
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

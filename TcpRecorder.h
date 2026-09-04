#pragma once
#include <Arduino.h>
#include <LittleFS.h>

class TcpRecorder {
public:
    TcpRecorder();
    bool begin();
    
    // Startet die Aufnahme und öffnet eine Datei mit dem gewünschten Namen
    bool startRecording(const String& fileName);
    void stopRecording();
    
    // Diese Funktion wird jetzt im "writeSpi" des ADAU-Treibers aufgerufen
    void recordSpiWrite(uint16_t address, const uint8_t* data, uint32_t length);
    
    // Lädt einen Mitschnitt und gibt ihn direkt an die writeSpi-Funktion des ADAU weiter
    bool playMacro(const String& fileName, bool (*writeSpiCallback)(uint16_t, const uint8_t*, uint32_t));

    void listFiles();
    String getFileNameByIndex(uint32_t targetIndex);
    static bool isRecording;

private:
    File _recordFile;
    String _currentFileName;
};

extern TcpRecorder tcpRecorder;

# ESP32 SigmaStudio TCP/IP to SPI Bridge for ADAU1467

Choose your language / Wählen Sie Ihre Sprache:
* [English Documentation](#english-documentation)
* [Deutsche Dokumentation](#deutsche-dokumentation)

---

## English Documentation

This project allows you to program, tune, and debug an **Analog Devices ADAU1467 DSP** (or pin-compatible variants like ADAU1463/1452) completely wirelessly over Wi-Fi directly from **SigmaStudio** or via remote network terminals.

The ESP32 acts as a high-performance bridge. It emulates SigmaStudio's native TCP protocol, forwards commands via SPI-DMA (Direct Memory Access), and provides a comprehensive multi-client console framework.

### Features & Architecture Updates
* **Smart Software Autoboot (No Hardware Selfboot Pin Needed):** If a boot macro is saved under the name `boot_prog.bin` using the TcpRecorder, the `Adau146xController` will automatically detect, load, and initialize the ADAU1467 on startup. No physical `SELFBOOT` pin wiring or hardware pin pulling is required on the DSP side!
* **Dead-Simple Code Integration:** To integrate the bridge into any existing project, simply instantiate the `Adau146xController` class and call its `.run()` method in your main loop.
* **Centralized Configuration:** All hardware pin assignments, network parameters, and system behaviors are cleanly separated and easily adjustable inside a single file: `Config.h`.
* **Telnet / Putty Remote CLI:** In addition to the local Serial Monitor, you can now connect wirelessly via **Putty** (or any Telnet client) to manage your DSP over the network.
* **Direct Register Control (`set_reg` / `get_reg`):** Read and write DSP memory registers on-the-fly directly from the command line interface.
* **Native SigmaStudio TCP Target:** No modifications required in SigmaStudio (uses the original Link Workstation architecture).
* **Real-Time Tuning:** Zero-latency adjustments of EQ sliders, volume, etc., over Wi-Fi.
* **Autonomous SPI Recorder (LittleFS):** Record entire compilation downloads or individual presets directly at the hardware SPI layer, store them in the ESP32's flash memory, and replay them anytime via numbers/names without a PC connection.
* **FreeRTOS Multithreading:** Network I/O and SPI communication run isolated on separate cores – no blocking, maximum stability.
* **High-Speed SPI with DMA:** Safely transfers even large firmware files without buffer overflows.

### Hardware Requirements (Important!)

This project was specifically designed for powerful ESP32 variants. Due to the large amount of data transferred when flashing the DSP, the code relies heavily on external PSRAM.

* **Recommended Board:** ESP32-S3 DevKit (developed and tested on an **ESP32-S3 N32R8V** with 32 MB Flash and 8 MB Octal-PSRAM).
* **Requirement:** Your board **MUST** have external **PSRAM**, as the TCP buffers are allocated in SPIRAM. Boards without PSRAM will encounter memory allocation failures (crash).

### Hardware Wiring (Pinout)

Pins can be customized centrally in `Config.h`. By default, the following pinout applies to the ESP32:

| Signal | ESP32 Pin | ADAU1467 Pin | Description |
| :--- | :--- | :--- | :--- |
| **MOSI** | 11 | MOSI / SDATA_IN | Data from ESP32 to DSP |
| **MISO** | 13 | MISO / SDATA_OUT| Data from DSP to ESP32 |
| **SCLK** | 12 | SCLK / SCK | SPI Clock line |
| **CS**   | 10 | SS / CS | Chip Select |
| **GND**  | GND | GND | Common Ground (Crucial!) |

*Note: Make sure the ADAU1467 is configured for SPI Slave Mode via its hardware configuration pins so it listens to the ESP32.*

### Software Setup

1. Clone or download this repository.
2. Create a file named `Credentials.h` in the same directory (ignored by `.gitignore`) and enter your Wi-Fi credentials:
   ```cpp
   #pragma once
   constexpr char WIFI_SSID[] = "YOUR_WIFI_NAME";
   constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
   ```
3. Open the project in your development environment (Arduino IDE or PlatformIO) and flash it to your ESP32.
4. Open the Serial Monitor (115200 Baud). Once connected to Wi-Fi, the ESP32 will display its **IP address** and load the active command dashboard.

### Firmware Integration Example

Integrating the bridge into your main software loop is straightforward:

```cpp
#include "Adau146xController.h"

// Instantiate the controller globally
Adau146xController dspBridge;

void setup() {
    Serial.begin(115200);
    // Ensure Wi-Fi/LittleFS is initialized as required by your project
}

void loop() {
    // Keep the TCP server, SPI recorder, and Putty console processing active
    dspBridge.run();
}
 ```

### Serial & Putty CLI Control Dashboard
 
Using the Serial Monitor or a Putty connection (Port configured in `Config.h`, e.g., `8087`), you can execute the following commands:

* `list` : Refreshes and displays the menu interface and all saved binary programs inside LittleFS.
* `rec_on [Name]` : Starts recording all incoming hardware SPI write commands (Default name: `macro`). Hint: Record as `boot_prog` to enable standalone software autoboot.
* `rec_off` : Stops the current recording and automatically lists the saved file inside the LittleFS system.
* `replay [Number]` : Instantly plays back a macro or full boot program by its list index number (e.g., `replay 1`).
* `replay [Name]` : Alternative method to trigger a replay directly via its file name string.
* `set_reg <address> <byte1> [byte2] ...` : Manually writes data bytes to a specific DSP register address (Hex/Dec format supported).
* `get_reg <address> <length>` : Reads a specified number of data bytes from a DSP register address and dumps them as HEX.
* `tcp_log_on` / `tcp_log_off` : Toggles real-time hex dissection and printouts of network packets.
* `status` : Displays current logging activities and recorder states.
* `exit` / `quit` : Safely disconnects the current Putty/network console session.

### Configuration in SigmaStudio

To use the wireless connection, adjust your setup in SigmaStudio as follows:

1. Open your project and navigate to the **Hardware Configuration** tab.
2. Delete the default USBi interface from the white workspace, if present.
3. In the Toolbox on the left, search for **TCPIP** (under *Communication Channels*).
4. Drag the **TCPIP** module into the workspace and connect it to your ADAU1467 block.
5. Right-click the TCPIP module and select **Properties** (or click the IP address on the module).
6. Enter the following details:
   * **IP Address:** *The IP address of your ESP32 shown in the Serial Monitor*
   * **Port:** `8086`
7. Once connected, the status bar at the bottom of SigmaStudio will turn green ("Ready"), and you can click "Link Compile Download" as usual.

### Credits & Acknowledgements
* **Protocol Specification:** This project implements the official TCP/IP communication format documented by **Analog Devices, Inc.** for ADAU144x/ADAU145x/ADAU146x channels. It is an independent implementation and is not affiliated with or endorsed by Analog Devices.
* **Frameworks:** Built using the [Arduino core for ESP32](https://github.com) and Espressif's ESP-IDF FreeRTOS components.

---

## Deutsche Dokumentation

Dieses Projekt ermöglicht es, einen **Analog Devices ADAU1467 DSP** (oder baugleiche wie ADAU1463/1452) komplett kabellos über WLAN direkt aus **SigmaStudio** oder Remote-Netzwerk-Terminals heraus zu programmieren und zu debuggen. 

Der ESP32 fungiert als performante Bridge. Er emuliert das native TCP-Protokoll von SigmaStudio, leitet die Befehle via SPI-DMA (Direct Memory Access) an den DSP weiter und bietet ein umfassendes Multi-Client-Konsolen-Framework.

### Features & Architektur-Updates
* **Intelligenter Software-Autoboot (Kein Hardware-Selfboot-Pin nötig):** Wenn ein Firmware-Boot-Makro unter dem Namen `boot_prog.bin` mithilfe des TcpRecorders aufgezeichnet wurde, erkennt der `Adau146xController` dies beim Starten automatisch, lädt es und initialisiert den ADAU1467 vollständig autark. Ein physischer `SELFBOOT`-Pin oder Hardware-Pull-Up/Downs am DSP sind für den Standalone-Betrieb nicht mehr erforderlich!
* **Kinderleichte Code-Integration:** Zur Einbindung in ein bestehendes Projekt muss lediglich eine Instanz der Klasse `Adau146xController` erzeugt und deren Funktion `.run()` in der Hauptschleife aufgerufen werden.
* **Zentrale Konfiguration:** Alle Hardware-Pins, Netzwerk-Parameter und Systemoptionen stehen übersichtlich und zentral gebündelt in der Datei `Config.h`.
* **Putty / Telnet Remote-CLI:** Neben dem klassischen Seriellen Monitor kannst du dich nun auch per **Putty** (oder anderen Telnet-Clients) drahtlos auf den Mikrocontroller aufschalten, um ihn im Netzwerk fernzusteuern.
* **Direkte Register-Manipulation (`set_reg` / `get_reg`):** Über die Konsole können Register im ADAU-DSP zur Laufzeit direkt ausgelesen und beschrieben werden.
* **Natives SigmaStudio TCP-Target:** Keine Modifikation an SigmaStudio nötig (nutzt die originale Link-Workstation-Architektur).
* **Echtzeit-Tuning:** Latenzfreies Verschieben von EQ-Reglern, Lautstärke etc. via WLAN.
* **Autarker SPI-Recorder (LittleFS):** Schneidet komplette Kompilierungs-Downloads oder dedizierte Einstellungen direkt auf der Hardware-SPI-Schicht mit, legt sie im Flash-Speicher des ESP32 ab und führt sie jederzeit per Index-Auswahl völlig autark ohne PC aus.
* **FreeRTOS Multithreading:** Netzwerk-I/O und SPI-Kommunikation laufen isoliert auf eigenen Kernen – kein Blockieren, maximale Stabilität.
* **High-Speed SPI mit DMA:** Sicheres Übertragen selbst großer Firmware-Dateien ohne Pufferüberlauf.

### Hardware-Anforderungen (Wichtig!)

Dieses Projekt wurde speziell für leistungsstarke ESP32-Varianten entwickelt. Aufgrund der großen Datenmengen beim Flashen des DSPs nutzt der Code intensiv den externen PSRAM.
•	Empfohlenes Board: ESP32-S3 DevKit (entwickelt und getestet auf einem ESP32-S3 N32R8V mit 32 MB Flash und 8 MB Octal-PSRAM).
•	Voraussetzung: Das Board MUSS über externen PSRAM verfügen, da die TCP-Buffer im SPIRAM allokiert werden. Boards ohne PSRAM führen zu einem Speicherüberlauf (Crash).
Hardware-Verkabelung (Pinout)
Die Pins können zentral in der Config.h angepasst werden. Standardmäßig gilt folgende Belegung für den ESP32:
Signal	ESP32 Pin	ADAU1467 Pin	Beschreibung
MOSI	11	MOSI / SDATA_IN	Daten vom ESP32 zum DSP
MISO	13	MISO / SDATA_OUT	Daten vom DSP zum ESP32
SCLK	12	SCLK / SCK	SPI Taktleitung
CS	10	SS / CS	Chip Select
GND	GND	GND	Gemeinsame Masse (Wichtig!)
Hinweis: Achte darauf, dass der ADAU1467 über seine Hardware-Konfigurations-Pins für den SPI-Slave-Modus eingestellt ist, damit er auf die Befehle des ESP32 lauscht.
Software-Einrichtung
1.	Klone oder downloade dieses Repository.
2.	Erstelle im selben Ordner eine Datei namens Credentials.h (wird von .gitignore ignored) und trage deine WLAN-Daten ein:
cpp #pragma once constexpr char WIFI_SSID[] = "DEIN_WLAN_NAME"; constexpr char WIFI_PASSWORD[] = "DEIN_WLAN_PASSWORT"; 
3.	Öffne das Projekt in deiner Entwicklungsumgebung (Arduino IDE oder PlatformIO) und flashe es auf deinen ESP32.
4.	Öffne den Seriellen Monitor (115200 Baud). Sobald sich der ESP32 verbunden hat, wird dir seine IP-Adresse sowie das interaktive Befehlsmenü angezeigt.
Beispiel zur Code-Integration
Die Einbindung der Bridge in dein bestehendes Hauptprogramm erfordert nur minimale Zeilen:
```cpp
#include "Adau146xController.h"
// 1. Die Klasse adau146XController global erzeugen
Adau146xController dspBridge;
void setup() {
Serial.begin(115200);
// Ggf. WLAN und LittleFS starten, falls nicht in der Klasse gekapselt
}
void loop() {
// 2. Die Funktion run() zyklisch aufrufen, um Server, Recorder und Putty zu verarbeiten
dspBridge.run();
}
```
Serielle & Putty Terminal-Steuerung
Über den Seriellen Monitor oder eine Putty-Verbindung (Port in der Config.h einstellbar, standardmäßig z.B. 8087) stehen dir folgende CLI-Befehle zur Verfügung:
•	list : Aktualisiert die Anzeige und listet alle im LittleFS-Speicher vorhandenen Binärprogramme auf.
•	rec_on [Name] : Startet die Aufzeichnung aller Hardware-SPI-Schreibbefehle unter dem gewünschten Namen (Standardname: macro). Tipp: Zeichne ein Programm als boot_prog auf, um den automatischen Software-Autoboot beim Systemstart zu aktivieren.
•	rec_off : Stoppt die aktuelle Aufzeichnung und speichert die Datei permanent im Flash ab.
•	replay [Nummer] : Spielt das gespeicherte Makro oder Boot-Programm komfortabel über seine Menü-Indexnummer ab (z. B. replay 1).
•	replay [Name] : Führt alternativ das Replay direkt über die Eingabe des genauen Dateinamens aus.
•	set_reg <Adresse> <Byte1> [Byte2] ... : Schreibt manuelle Datenbytes auf eine spezifische Registeradresse des DSPs (Hexadezimal- oder Dezimal-Format).
•	get_reg <Adresse> <Länge> : Liest die gewünschte Anzahl an Bytes von einer Registeradresse des ADAU aus und gibt sie als HEX-Dump aus.
•	tcp_log_on / tcp_log_off : Aktiviert/Deaktiviert das tiefe Live-Sezieren von Netzwerkpaketen im Hintergrund.
•	status : Zeigt den momentanen Zustand von Aufzeichnung und Netzwerk-Logging an.
•	exit / quit / logout : Trennt die aktive Putty-/Netzwerk-Konsolenverbindung sauber.
Konfiguration in SigmaStudio
Um die kabellose Verbindung zu nutzen, passe dein Setup in SigmaStudio wie folgt an:
1.	Öffne dein Projekt und gehe zum Hardware Configuration-Tab.
2.	Lösche das standardmäßige USBi-Interface aus dem weißen Fenster, falls vorhanden.
3.	Suche in der Toolbox auf der linken Seite nach TCPIP (unter Communication Channels).
4.	Ziehe das TCPIP-Modul in das Fenster und verbinde es mit deinem ADAU1467-Baustein.
5.	Klicke mit der rechten Maustaste auf das TCPIP-Modul und wähle Properties (oder klicke auf die IP-Adresse im Modul).
6.	Trage dort die Daten ein:
o	IP Address: Die IP-Adresse deines ESP32 aus dem Seriellen Monitor
o	Port: 8086
7.	Sobald die Verbindung steht, färbt sich die Statusleiste unten in SigmaStudio grün ("Ready") und du kannst wie gewohnt auf "Link Compile Download" drücken.


## License
This project is licensed under the MIT License - see the `LICENSE` file for details.

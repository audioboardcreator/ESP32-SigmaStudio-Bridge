# ESP32 SigmaStudio TCP/IP to SPI Bridge for ADAU1467

Choose your language / Wählen Sie Ihre Sprache:
* [English Documentation](#english-documentation)
* [Deutsche Dokumentation (Hier klicken / Click here)](#deutsche-dokumentation)

---

## English Documentation

This project allows you to program and tune an **Analog Devices ADAU1467 DSP** (or pin-compatible variants like ADAU1463/1452) completely wirelessly over Wi-Fi directly from **SigmaStudio**.

The ESP32 acts as a high-performance bridge. It emulates SigmaStudio's native TCP protocol and forwards commands to the DSP via SPI-DMA (Direct Memory Access).

### Features
* **Native SigmaStudio TCP Target:** No modifications required in SigmaStudio (uses the original Link Workstation architecture).
* **Real-Time Tuning:** Zero-latency adjustments of EQ sliders, volume, etc., over Wi-Fi.
* **FreeRTOS Multithreading:** Network I/O and SPI communication run isolated on separate cores – no blocking, maximum stability.
* **High-Speed SPI with DMA:** Safely transfers even large firmware files without buffer overflows.

### Hardware Requirements (Important!)

This project was specifically designed for powerful ESP32 variants. Due to the large amount of data transferred when flashing the DSP, the code relies heavily on external PSRAM.

* **Recommended Board:** ESP32-S3 DevKit (developed and tested on an **ESP32-S3 N32R8V** with 32 MB Flash and 8 MB Octal-PSRAM).
* **Requirement:** Your board **MUST** have external **PSRAM**, as the TCP buffers are allocated in SPIRAM. Boards without PSRAM will encounter memory allocation failures (crash).

### Hardware Wiring (Pinout)

Pins can be customized in `Config.h`. By default, the following pinout applies to the ESP32:

| Signal | ESP32 Pin | ADAU1467 Pin | Description |
| :--- | :--- | :--- | :--- |
| **MOSI** | 11 | MOSI / SDATA_IN | Data from ESP32 to DSP |
| **MISO** | 13 | MISO / SDATA_OUT| Data from DSP to ESP32 |
| **SCLK** | 12 | SCLK / SCK | SPI Clock line |
| **CS**   | 10 | SS / CS | Chip Select |
| **GND**  | GND | GND | Common Ground (Crucial!) |

*Note: Make sure the ADAU1467 is correctly configured for SPI Slave Mode via its hardware pins (Selfboot pins, etc.).*

### Software Setup

1. Clone or download this repository.
2. Create a file named `Credentials.h` in the same directory (ignored by `.gitignore`) and enter your Wi-Fi credentials:
   ```cpp
   #pragma once
   constexpr char WIFI_SSID[] = "YOUR_WIFI_NAME";
   constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
   ```
3. Open the project in your development environment (Arduino IDE or PlatformIO) and flash it to your ESP32.
4. Open the Serial Monitor (115200 Baud). Once connected to Wi-Fi, the ESP32 will display its **IP address**.

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

<details>
<summary><b>📐 KLICKE HIER, UM DIE DEUTSCHE ANLEITUNG EINZUBLENDEN (CLICK TO EXPAND)</b></summary>

<br>

Dieses Projekt ermöglicht es, einen **Analog Devices ADAU1467 DSP** (oder baugleiche wie ADAU1463/1452) komplett kabellos über WLAN direkt aus **SigmaStudio** heraus zu programmieren und in Echtzeit zu tunen. 

Der ESP32 fungiert als performante Bridge. Er emuliert das native TCP-Protokoll von SigmaStudio und leitet die Befehle via SPI-DMA (Direct Memory Access) an den DSP weiter.

### Features
* **Natives SigmaStudio TCP-Target:** Keine Modifikation an SigmaStudio nötig (nutzt die originale Link-Workstation-Architektur).
* **Echtzeit-Tuning:** Latenzfreies Verschieben von EQ-Reglern, Lautstärke etc. via WLAN.
* **FreeRTOS Multithreading:** Netzwerk-I/O und SPI-Kommunikation laufen isoliert auf eigenen Kernen – kein Blockieren, maximale Stabilität.
* **High-Speed SPI mit DMA:** Sicheres Übertragen selbst großer Firmware-Dateien ohne Pufferüberlauf.

### Hardware-Anforderungen (Wichtig!)

Dieses Projekt wurde speziell für leistungsstarke ESP32-Varianten entwickelt. Aufgrund der großen Datenmengen beim Flashen des DSPs nutzt der Code intensiv den externen PSRAM.

* **Empfohlenes Board:** ESP32-S3 DevKit (entwickelt und getestet auf einem **ESP32-S3 N32R8V** mit 32 MB Flash und 8 MB Octal-PSRAM).
* **Voraussetzung:** Das Board **MUSS** über externen **PSRAM** verfügen, da die TCP-Buffer im SPIRAM allokiert werden. Boards ohne PSRAM führen zu einem Speicherüberlauf (Crash).

### Hardware-Verkabelung (Pinout)

Die Pins können in der `Config.h` angepasst werden. Standardmäßig gilt folgende Belegung für den ESP32:

| Signal | ESP32 Pin | ADAU1467 Pin | Beschreibung |
| :--- | :--- | :--- | :--- |
| **MOSI** | 11 | MOSI / SDATA_IN | Daten vom ESP32 zum DSP |
| **MISO** | 13 | MISO / SDATA_OUT| Daten vom DSP zum ESP32 |
| **SCLK** | 12 | SCLK / SCK | SPI Taktleitung |
| **CS**   | 10 | SS / CS | Chip Select |
| **GND**  | GND | GND | Gemeinsame Masse (Wichtig!) |

*Hinweis: Achte darauf, dass der ADAU1467 für den SPI-Slave-Modus korrekt über seine eigenen Hardware-Pins (Selfboot-Pins etc.) konfiguriert ist.*

### Software-Einrichtung

1. Klone oder downloade dieses Repository.
2. Erstelle im selben Ordner eine Datei namens `Credentials.h` (wird von `.gitignore` ignored) und trage deine WLAN-Daten ein:
   ```cpp
   #pragma once
   constexpr char WIFI_SSID[] = "DEIN_WLAN_NAME";
   constexpr char WIFI_PASSWORD[] = "DEIN_WLAN_PASSWORT";
   ```
3. Öffne das Projekt in deiner Entwicklungsumgebung (Arduino IDE oder PlatformIO) und flashe es auf deinen ESP32.
4. Öffne den Seriellen Monitor (115200 Baud). Sobald sich der ESP32 verbunden hat, wird dir seine **IP-Adresse** angezeigt.

### Konfiguration in SigmaStudio

Um die kabellose Verbindung zu nutzen, passe dein Setup in SigmaStudio wie folgt an:

1. Öffne dein Projekt und gehe zum **Hardware Configuration**-Tab.
2. Lösche das standardmäßige USBi-Interface aus dem weißen Fenster, falls vorhanden.
3. Suche in der Toolbox auf der linken Seite nach **TCPIP** (unter *Communication Channels*).
4. Ziehe das **TCPIP**-Modul in das Fenster und verbinde es mit deinem ADAU1467-Baustein.
5. Klicke mit der rechten Maustaste auf das TCPIP-Modul und wähle **Properties** (oder klicke auf die IP-Adresse im Modul).
6. Trage dort die Daten ein:
   * **IP Address:** *Die IP-Adresse deines ESP32 aus dem Seriellen Monitor*
   * **Port:** `8086`
7. Sobald die Verbindung steht, färbt sich die Statusleiste unten in SigmaStudio grün ("Ready") und du kannst wie gewohnt auf "Link Compile Download" drücken.

### Hinweise & Danksagungen
* **Protokoll-Spezifikation:** Dieses Projekt implementiert das offizielle TCP/IP-Kommunikationsformat von **Analog Devices, Inc.** für ADAU144x/ADAU145x/ADAU146x Kanäle. Dies ist eine unabhängige Implementierung; es besteht keine Verbindung zu Analog Devices.
* **Frameworks:** Entwickelt auf Basis des [Arduino core für ESP32](https://github.com) und den FreeRTOS-Komponenten des ESP-IDF von Espressif.

</details>

---

## License
This project is licensed under the MIT License - see the `LICENSE` file for details.

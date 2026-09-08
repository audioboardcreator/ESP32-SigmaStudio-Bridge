#pragma once
#include <Arduino.h>
#include "driver/spi_master.h"
#include "Credentials.h"
 
constexpr uint16_t SERVER_PORT  = 8086;
constexpr uint16_t CONSOLE_PORT = 8087;

constexpr uint32_t CONSOLE_BAUDRATE = 115200;
constexpr uint32_t MAIN_LOOP_DELAY_MS = 50;

constexpr int PIN_BT_POWER = 4;
constexpr uint32_t BT_POWER_PULSE_MS = 3000;

constexpr BaseType_t TCP_TASK_CORE  = 1;
constexpr BaseType_t ADAU_TASK_CORE = 1;

constexpr UBaseType_t TCP_TASK_PRIORITY  = 2;
constexpr UBaseType_t ADAU_TASK_PRIORITY = 1;

constexpr UBaseType_t RX_QUEUE_LENGTH = 2;
constexpr UBaseType_t TX_QUEUE_LENGTH = 2;

constexpr spi_host_device_t ADAU_SPI_HOST = SPI2_HOST;

constexpr int PIN_MOSI = 11;
constexpr int PIN_MISO = 13;
constexpr int PIN_SCLK = 12;
constexpr int PIN_CS   = 10;

constexpr int PIN_ONBOARD_LED = 38;

constexpr uint32_t ADAU_SPI_CLOCK_HZ = 20000000;

constexpr char AUTOBOOT_MACRO[] = "boot_prog";
constexpr char AUTOBOOT_FILE[]  = "/boot_prog.bin";

constexpr BaseType_t LOGGER_TASK_CORE = 1;
constexpr UBaseType_t LOGGER_TASK_PRIORITY = 1;

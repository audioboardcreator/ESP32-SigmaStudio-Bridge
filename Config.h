#pragma once

#include <Arduino.h>
#include "driver/spi_master.h"
#include "Credentials.h"



/*
 * TCP-Konfiguration
 */
constexpr uint16_t SERVER_PORT =
    8086;

/*
 * Task-Konfiguration
 */
constexpr BaseType_t TCP_TASK_CORE =
    1;

constexpr BaseType_t ADAU_TASK_CORE =
    1;

constexpr UBaseType_t TCP_TASK_PRIORITY =
    2;

constexpr UBaseType_t ADAU_TASK_PRIORITY =
    1;

/*
 * Queue-Konfiguration
 */
constexpr UBaseType_t RX_QUEUE_LENGTH =
    2;

constexpr UBaseType_t TX_QUEUE_LENGTH =
    2;

/*
 * SPI-Host
 */
constexpr spi_host_device_t ADAU_SPI_HOST =
    SPI2_HOST;

/*
 * SPI-Pins
 */
constexpr int PIN_MOSI =
    11;

constexpr int PIN_MISO =
    13;

constexpr int PIN_SCLK =
    12;

constexpr int PIN_CS =
    10;

/*
 * SPI-Takt: zunächst 1 MHz
 */
constexpr uint32_t ADAU_SPI_CLOCK_HZ =
    1000000;
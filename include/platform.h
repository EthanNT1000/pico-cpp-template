#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include "mcp2515.h"

#define WIFI_SSID "usv-002"
#define WIFI_PASSWORD "43819327"
#define ROS_UDP_PORT 8888  // Default micro-ROS agent UDP port
#define ROS_AGENT_IP "10.42.0.1"  // Replace with your agent's IP

#define ENCODER_POLL_PERIOD_MS 250

#define MCP2515_SPI_PORT spi0
#define MCP2515_SPI_BAUDRATE 10000000
#define MCP2515_PIN_CS   5
#define MCP2515_PIN_SCK  6
#define MCP2515_PIN_MISO 4
#define MCP2515_PIN_MOSI 7
#define MCP2515_PIN_INT  21
#define MCP2515_OSCILLATOR Mcp2515::Oscillator::MHZ16
#define MCP2515_BITRATE Mcp2515::Bitrate::KBPS1000

#define ENCODER_SPI_PORT spi1
#define ENCODER_SPI_BAUDRATE 10000
#define ENCODER_SPI_CS 13
#define ENCODER_SPI_SCK 10
#define ENCODER_SPI_MISO 12
#define ENCODER_SPI_MOSI 11

#define MAVLINK_UART_PORT uart1
#define MAVLINK_UART_BAUDRATE 57600
#define MAVLINK_UART_TX 8
#define MAVLINK_UART_RX 9

#endif

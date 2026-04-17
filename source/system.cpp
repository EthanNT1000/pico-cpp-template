#include "system.h"
#include "FreeRTOS.h"

#include <stdio.h>
#include "platform.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "dma.h"

#include "flash_storage.h"
#include "pico/flash.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

SPI* mcp2515Spi;
Mcp2515* mcp2515;
uint8_t nodeId;

void initSystem() {
    stdio_init_all();
    DMA::resetCallbackList();

    flash_storage_read(&nodeId);
    printf("Node ID = %d\n", nodeId);

    mcp2515Spi = new SPI(MCP2515_SPI_PORT, MCP2515_SPI_BAUDRATE,
        MCP2515_PIN_CS, MCP2515_PIN_SCK,
        MCP2515_PIN_MOSI, MCP2515_PIN_MISO, false,
        SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, true);
    mcp2515 = new Mcp2515(mcp2515Spi, MCP2515_PIN_INT, MCP2515_OSCILLATOR, MCP2515_BITRATE);
}

// CRC-16/CCITT-FALSE polynomial 0x1021, initial 0xFFFF
static uint16_t calculate_crc(const uint8_t * data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static const uint8_t * flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_STORAGE_OFFSET);

bool flash_storage_read(uint8_t * node_id) {
    const flash_storage_t * stored = (const flash_storage_t *)flash_ptr;

    // Validate header
    if (stored->header != FLASH_STORAGE_HEADER) {
        return false;  // Invalid or uninitialized
    }

    // Compute CRC over header + node_id + padding
    uint16_t expected_crc = calculate_crc((const uint8_t *)stored, offsetof(flash_storage_t, crc));
    if (expected_crc != stored->crc) {
        return false;  // Corrupted data
    }

    *node_id = stored->node_id;
    return true;
}

// This function will be called when it's safe to call flash_range_erase
static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

// This function will be called when it's safe to call flash_range_program
static void call_flash_range_program(void *param) {
    uint32_t offset = (reinterpret_cast<uintptr_t*>(param))[0];
    const uint8_t *data = (const uint8_t *)(reinterpret_cast<uintptr_t*>(param))[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}


void flash_storage_write(uint8_t node_id) {
    flash_storage_t data = {0};
    data.header   = FLASH_STORAGE_HEADER;
    data.node_id  = node_id;
    data.padding  = 0xFF;  // Optional fill

    // Calculate CRC over header + node_id + padding
    data.crc = calculate_crc((const uint8_t *)&data, offsetof(flash_storage_t, crc));

    // Erase and program the sector
    flash_safe_execute(call_flash_range_erase, reinterpret_cast<void*>(FLASH_STORAGE_OFFSET), UINT32_MAX);

    uintptr_t params[] = { FLASH_STORAGE_OFFSET, (uintptr_t)&data};
    flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
}

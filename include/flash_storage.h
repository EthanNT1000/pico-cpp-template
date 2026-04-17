#pragma once

#include <stdint.h>

// Magic header value to identify valid data (choose a unique constant)
#define FLASH_STORAGE_HEADER 0x524E  // "RN" in ASCII, or any preferred value

// Reserved flash sector configuration (from previous setup)
#define FLASH_SECTOR_SIZE    4096U
#define FLASH_STORAGE_OFFSET (2 * 1024 * 1024 - FLASH_SECTOR_SIZE)  // Last 4 KB

typedef struct {
    uint16_t header;   // Magic value for validation
    uint8_t  node_id;  // Your node ID
    uint8_t  padding;  // Align to 4-byte boundary (optional but recommended)
    uint16_t crc;      // CRC-16 over header + node_id (+ padding if used)
} flash_storage_t;

// Static assertion for size (ensures structure fits comfortably)
static_assert(sizeof(flash_storage_t) <= 8, "Structure too large");

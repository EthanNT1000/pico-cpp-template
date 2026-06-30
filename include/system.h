#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <cstdint>
#include "spi.h"
#include "mcp2515Task.h"

extern SPI* mcp2515Spi;
extern uint8_t nodeId;

enum class TransmitDataType : uint8_t {

};

typedef struct TransmitMsg_t {
    TransmitDataType type;
    union {
    };
} TransmitMsg_t;

void initSystem();
bool flash_storage_read(uint8_t* node_id);
void flash_storage_write(uint8_t node_id);
#endif

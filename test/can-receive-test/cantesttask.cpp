#include "cantesttask.h"
#include <stdio.h>

CANTestTask::CANTestTask(Mcp2515Task* mcp2515, const char* pcName,
    configSTACK_DEPTH_TYPE usStackDepth, Priority uxPriority,
    Mcp2515::RxBufferType rxMode)
    : _mcp2515(mcp2515), _rxMode(rxMode),
      TaskInterface(pcName, usStackDepth, uxPriority) {
}

void CANTestTask::run() {
    _mcp2515->initialInTask(_rxMode);
    printf("[CAN TEST] Ready. Mode: %s\n",
        _rxMode == Mcp2515::RxBufferType::Rollover ? "Rollover" : "Independent");

    while (1) {
        uint32_t canId;
        uint8_t buf[8];
        uint8_t len;

        _mcp2515->rxReadInTask(&canId, buf, &len);

        printf("[CAN RX] ID:0x%03lX LEN:%d DATA:", canId, len);
        for (uint8_t i = 0; i < len; i++) {
            printf(" %02X", buf[i]);
        }
        printf("\n");

        // Echo back with ID + 1 so the Python script can verify receipt
        uint32_t echoId = canId | 0x400;  // high bit set — guaranteed no collision with sender IDs
        _mcp2515->txBuffer0SendInTask(echoId, buf, len);
        printf("[CAN TX] Echo ID:0x%03lX\n", echoId);
    }
}

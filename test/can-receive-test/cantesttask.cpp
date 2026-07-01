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

        // ID 0x000 is the control channel — always accepted (default filter 1 = 0x000).
        // data[0]=0x00 → accept-all  0x01 → std filter test  0x02 → ext filter test
        if (canId == 0x000 && len >= 1) {
            if (buf[0] == 0x01) {
                _mcp2515->setMask(0, 0x7FF);             _mcp2515->setFilter(0, 0x100);
                _mcp2515->setMask(1, 0x7FF);             _mcp2515->setFilter(2, 0x200);
                printf("[CAN TEST] std filter ON (RXB0=0x100 RXB1=0x200)\n");
            } else if (buf[0] == 0x02) {
                _mcp2515->setMask(0, 0x1FFFFFFF, true);  _mcp2515->setFilter(0, 0x1ABCDEF, true);
                _mcp2515->setMask(1, 0x1FFFFFFF, true);  _mcp2515->setFilter(2, 0x1234567, true);
                printf("[CAN TEST] ext filter ON (RXB0=0x1ABCDEF RXB1=0x1234567)\n");
            } else if (buf[0] == 0x00) {
                _mcp2515->setMask(0, 0x000);
                _mcp2515->setMask(1, 0x000);
                printf("[CAN TEST] filter OFF (accept-all)\n");
            }
        }

        uint32_t echoId = canId | 0x400;
        _mcp2515->txBuffer0SendInTask(echoId, buf, len);
        printf("[CAN TX] Echo ID:0x%03lX\n", echoId);
    }
}

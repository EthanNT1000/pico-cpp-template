#include "cansendtask.h"
#include "mcp2515reg.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "taskhandler.h"
#include "system.h"

CANSendTask::CANSendTask(Mcp2515Task* mcp2515, const char* pcName, const configSTACK_DEPTH_TYPE usStackDepth,
    Priority uxPriority, uint8_t queueSize) : _mcp2515(mcp2515),
    TaskInterface(pcName, usStackDepth, uxPriority, xQueueCreate(queueSize, sizeof(TransmitMsg_t))) {
}

void CANSendTask::run() {
    _mcp2515->initialInTask();
    while (1) {
        TransmitMsg_t msg;
        xQueueReceive(rxQueue, &msg, portMAX_DELAY);

        if (nodeId == 0) {
            continue;
        }

        // if (msg.type == TransmitDataType::) {
        //     mcp2515->txBuffer0SendInTask(USV_PASEH0_CAN_XXX_DATA_ID(nodeId),
        //         reinterpret_cast<uint8_t*>(&msg.), sizeof(msg.));
        // }
    }
}

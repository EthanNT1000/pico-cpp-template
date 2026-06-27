#include "uarttesttask.h"
#include <stdio.h>

UartTestTask::UartTestTask(const char* pcName,
    const configSTACK_DEPTH_TYPE usStackDepth, Priority uxPriority)
    : TaskInterface(pcName, usStackDepth, uxPriority) {
    _uart = new UART(
        MAVLINK_UART_PORT,
        MAVLINK_UART_BAUDRATE,
        MAVLINK_UART_TX,
        MAVLINK_UART_RX,
        true,           // enableTxDma
        true,           // enableRxDma
        _rxBuffer,
        RX_HALF_SIZE
    );
}

void UartTestTask::run() {
    printf("UartTestTask started\n");
    while (1) {
        uint16_t size;
        const uint8_t* data = _uart->read(&size);
        if (!data || size == 0) continue;

        printf("RX %u bytes\n", size);
        _uart->write(data, size);
    }
}

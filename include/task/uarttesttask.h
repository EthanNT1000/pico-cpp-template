#ifndef _UART_TEST_TASK_H_
#define _UART_TEST_TASK_H_

#include "taskinterface.h"
#include "uart.h"
#include "platform.h"

class UartTestTask : public TaskInterface {
public:
    UartTestTask(const char* pcName, const configSTACK_DEPTH_TYPE usStackDepth,
                 Priority uxPriority);
    virtual ~UartTestTask() {}
    void run() override;

    void* operator new(size_t size)    { return pvPortMalloc(size); }
    void* operator new[](size_t size)  { return pvPortMalloc(size); }
    void operator delete(void* ptr)    { vPortFree(ptr); }
    void operator delete[](void* ptr)  { vPortFree(ptr); }

private:
    static constexpr uint16_t RX_HALF_SIZE = 64;

    UART* _uart;
    uint8_t _rxBuffer[RX_HALF_SIZE * 2];
};

#endif

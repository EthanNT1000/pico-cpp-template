#ifndef _CAN_TEST_TASK_H_
#define _CAN_TEST_TASK_H_

#include "taskinterface.h"
#include "mcp2515Task.h"

// Standalone CAN receive/echo test task.
// Calls initialInTask() itself — do NOT run alongside CANSendTask.
// Add to createTasks() and comment out canSendTask while testing.
class CANTestTask : public TaskInterface {
 public:
    CANTestTask(Mcp2515Task* mcp2515, const char* pcName,
        configSTACK_DEPTH_TYPE usStackDepth, Priority uxPriority,
        Mcp2515::RxBufferType rxMode = Mcp2515::RxBufferType::Rollover);
    virtual ~CANTestTask() {}
    void run() override;

    void* operator new(size_t size) { return pvPortMalloc(size); }
    void* operator new[](size_t size) { return pvPortMalloc(size); }
    void operator delete(void* ptr) { vPortFree(ptr); }
    void operator delete[](void* ptr) { vPortFree(ptr); }

 private:
    Mcp2515Task* _mcp2515;
    Mcp2515::RxBufferType _rxMode;
};

#endif

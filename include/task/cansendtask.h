#ifndef _MCP2515_TASK_H_
#define _MCP2515_TASK_H_

#include "taskinterface.h"
#include "mcp2515Task.h"

class CANSendTask : public TaskInterface {
 public:
     CANSendTask(Mcp2515Task* mcp2515, const char* pcName, const configSTACK_DEPTH_TYPE usStackDepth,
         Priority uxPriority, uint8_t queueSize);
     virtual ~CANSendTask() {}
     void run() override;

     void* operator new(size_t size) { return pvPortMalloc(size); }
     void* operator new[](size_t size) { return pvPortMalloc(size); }
     void operator delete(void* ptr) { vPortFree(ptr); }
     void operator delete[](void* ptr) { vPortFree(ptr); }

 private:
     Mcp2515Task* _mcp2515;
};
#endif

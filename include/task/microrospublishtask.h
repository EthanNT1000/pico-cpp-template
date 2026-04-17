#ifndef _MICRO_ROS_TASK_H_
#define _MICRO_ROS_TASK_H_

#include "taskinterface.h"
#include "spi.h"

class MicroRosPublishTask : public TaskInterface {
 public:
     MicroRosPublishTask(const char* pcName,
         const configSTACK_DEPTH_TYPE usStackDepth, Priority uxPriority, uint8_t queueSize);
     virtual ~MicroRosPublishTask() {}
    void run() override;

    void* operator new(size_t size)     {return pvPortMalloc(size);}
    void* operator new[](size_t size)   {return pvPortMalloc(size);}
    void operator delete(void * ptr)    {vPortFree(ptr);}
    void operator delete[](void* ptr) { vPortFree(ptr); }
};
#endif

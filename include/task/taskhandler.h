#ifndef _TASKHANDLER_H_
#define _TASKHANDLER_H_

#include "taskinterface.h"
#include <rclc/rclc.h>

extern rclc_support_t support;
extern rcl_node_t node;

void createTasks();
void startTasks();

void runTimeStats(void* arg);
void wifiConnectTask(void* arg);
void microRosMainTask(void* arg);
#endif

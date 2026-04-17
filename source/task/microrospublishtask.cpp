#include "microrospublishtask.h"
#include <rclc/rclc.h>
#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/u_int8.h>
#include <std_msgs/msg/bool.h>
#include <stdio.h>
#include <string>
#include "platform.h"
#include "system.h"
#include "taskhandler.h"

MicroRosPublishTask::MicroRosPublishTask(const char* pcName,
    const configSTACK_DEPTH_TYPE usStackDepth, Priority uxPriority, uint8_t queueSize) :
    TaskInterface(pcName, usStackDepth, uxPriority,
        xQueueCreate(queueSize, sizeof(TransmitMsg_t))) {
}

void MicroRosPublishTask::run() {
    ulTaskNotifyTake(true, portMAX_DELAY);

    // micro-ROS app
    // rcl_publisher_t xxxPublisher;
    // rclc_publisher_init_default(
    //     &xxxPublisher,
    //     &node,
    //     ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), "xxx");

    TransmitMsg_t msg {};
    while (1) {
        xQueueReceive(this->rxQueue, &msg, portMAX_DELAY);
        rcl_ret_t ret;

        // if (msg.type == TransmitDataType::) {
        //     ret = rcl_publish(&xxxPublisher, &msg., NULL);
        // }

        if (ret != RCL_RET_OK) {
            printf("Error publishing (line %d), ret = %d\n", __LINE__, ret);
        }
    }
}

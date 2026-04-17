#include "taskhandler.h"

#include <rclc/executor.h>
#include <uxr/client/transport.h>
#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/u_int8.h>
#include <std_msgs/msg/string.h>

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "pico/cyw43_arch.h"
#include "platform.h"
#include "system.h"
#include "udp_transport.h"
#include "pico/unique_id.h"

#include "microrospublishtask.h"
#include "cansendtask.h"

char unique_id_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
rclc_support_t support;
rcl_node_t node;
rcl_publisher_t nodeIdPublisher;

TaskHandle_t microRosMainTaskHandle;
MicroRosPublishTask* microRosPublishTask;
CANSendTask* canSendTask;

/**
 * @brief Handler in case our application overflows the stack
 *
 * @param xTask         Task handle
 * @param pcTaskName    Task name
 */
void vApplicationStackOverflowHook(
    TaskHandle_t xTask __attribute__((unused)),
    char *pcTaskName __attribute__((unused))) {
    printf("task %s is stack overflow. \r\n", pcTaskName);
    for (;;) {}
}

/**
 * @brief Start and run all freeRTOS tasks
 *
 */
void startTasks() {
     vTaskStartScheduler();
}

void createTasks() {
    // xTaskCreate(runTimeStats, "runTimeStats", 512, NULL, TaskInterface::Priority::Low, NULL);
    xTaskCreate(wifiConnectTask, "wifiConnectTask", 1024, NULL, TaskInterface::Priority::High, NULL);
    xTaskCreate(microRosMainTask, "microRosMainTask", 4096, NULL,
        TaskInterface::Priority::Low, &microRosMainTaskHandle);

    microRosPublishTask = new MicroRosPublishTask(
        "MicroRosPublishTask", 2048, TaskInterface::Priority::Medium, 100);
    canSendTask = new CANSendTask(mcp2515, "CANSendTask", 2048,
        TaskInterface::Priority::Medium, 100);
}

void runTimeStats(void *arg) {
    while (1) {
        TaskStatus_t* pxTaskStatusArray;
        volatile UBaseType_t uxArraySize, x;
        uint32_t ulTotalRunTime;

        // Get number of takss
        uxArraySize = uxTaskGetNumberOfTasks();
        printf("Number of tasks %d\n", uxArraySize);

        // Allocate a TaskStatus_t structure for each task.
        pxTaskStatusArray = reinterpret_cast<TaskStatus_t*>(pvPortMalloc(uxArraySize * sizeof(TaskStatus_t)));

        if (pxTaskStatusArray != NULL) {
            // Generate raw status information about each task.
            uxArraySize = uxTaskGetSystemState(pxTaskStatusArray,
                uxArraySize,
                &ulTotalRunTime);

            // Print stats
            for (x = 0; x < uxArraySize; x++) {
                printf("Task: %d \t cPri:%d \t bPri:%d \t hw:%d \t%s\n",
                    pxTaskStatusArray[x].xTaskNumber,
                    pxTaskStatusArray[x].uxCurrentPriority,
                    pxTaskStatusArray[x].uxBasePriority,
                    pxTaskStatusArray[x].usStackHighWaterMark,
                    pxTaskStatusArray[x].pcTaskName);
            }

            // Free array
            vPortFree(pxTaskStatusArray);
        } else {
            printf("Failed to allocate space for stats\n");
        }

        // Get heap allocation information
        HeapStats_t heapStats;
        vPortGetHeapStats(&heapStats);
        printf("HEAP avl: %d, blocks %d, alloc: %d, free: %d\n",
            heapStats.xAvailableHeapSpaceInBytes,
            heapStats.xNumberOfFreeBlocks,
            heapStats.xNumberOfSuccessfulAllocations,
            heapStats.xNumberOfSuccessfulFrees);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

static void wifiConnect() {
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA3_WPA2_AES_PSK)) {
        printf("failed to connect.\n");
    } else {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        uint8_t* ip_address = reinterpret_cast<uint8_t*>(&(cyw43_state.netif[0].ip_addr.addr));
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }
}

void wifiConnectTask(void* arg) {
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        assert(false);
    }

    // Enable wifi station
    cyw43_arch_enable_sta_mode();
    wifiConnect();
    xTaskNotifyGive(microRosMainTaskHandle);

    while (1) {
        if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP) {
            wifiConnect();
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// Custom allocation functions using FreeRTOS heap
static void * microros_allocate(size_t size, void * state) {
    (void)state;  // Unused in FreeRTOS heap
    return pvPortMalloc(size);
}

static void microros_deallocate(void * pointer, void * state) {
    (void)state;
    vPortFree(pointer);
}

static void * microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state) {
    (void)state;
    void * ptr = pvPortMalloc(number_of_elements * size_of_element);
    if (ptr) {
        memset(ptr, 0, number_of_elements * size_of_element);
    }
    return ptr;
}

static void* mrcroros_reallocate(void* pv, size_t xWantedSize, void* state) {
    (void)state;
        void * ptr = pvPortMalloc(xWantedSize);
    if (ptr) {
        memcpy(ptr, pv, xWantedSize);
        vPortFree(pv);
    }
    return ptr;
}


// Implementation example:
void subscription_callback(const void* msgin) {
    // Cast received message to used type
    const std_msgs__msg__UInt8* sub_msg = reinterpret_cast<const std_msgs__msg__UInt8*>(msgin);
    if (sub_msg->data > 0 && nodeId != sub_msg->data) {
        nodeId = sub_msg->data;
        flash_storage_write(nodeId);
        printf("Set Node ID to: %d\n", nodeId);
    }
}

void timer_callback(rcl_timer_t* timer, int64_t last_call_time) {
    rcl_ret_t ret = rcl_publish(&nodeIdPublisher, &nodeId, NULL);
    if (ret != RCL_RET_OK) {
        printf("Error publishing (line %d), ret = %d\n", __LINE__, ret);
    }
}

void microRosMainTask(void* arg) {
    ulTaskNotifyTake(true, portMAX_DELAY);

    rmw_uros_set_custom_transport(
        false,
        nullptr,
        udp_transport_open,
        udp_transport_close,
        udp_transport_write,
        udp_transport_read);

    rcl_allocator_t freeRTOS_allocator  = rcutils_get_zero_initialized_allocator();
    freeRTOS_allocator.allocate      = microros_allocate;
    freeRTOS_allocator.deallocate    = microros_deallocate;
    freeRTOS_allocator.reallocate    = mrcroros_reallocate;
    freeRTOS_allocator.zero_allocate = microros_zero_allocate;

    rcl_ret_t ret;
    do {
        ret = rclc_support_init(&support, 0, NULL, &freeRTOS_allocator);
        if (ret != RCL_RET_OK)
            printf("Error on rclc_support_init (line %d), ret = %d\n", __LINE__, ret);
    } while (ret != RCL_RET_OK);

    pico_get_unique_board_id_string(unique_id_str, sizeof(unique_id_str));

    // create node
    char name[19 + sizeof(unique_id_str)];
    snprintf(name, sizeof(name), "encoders/%s", unique_id_str);
    rclc_node_init_default(&node, unique_id_str, name, &support);

    rclc_executor_t executor;
    rcl_timer_t timer;
    rcl_subscription_t subscriber;

    rclc_publisher_init_default(
        &nodeIdPublisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt8),
        "node_id");

    rclc_executor_init(&executor, &support.context, 10, &freeRTOS_allocator);

    snprintf(name, sizeof(name), "setNodeId", unique_id_str);
    rcl_ret_t rc = rclc_subscription_init_default(
        &subscriber, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt8), name);
    if (RCL_RET_OK != rc) {
        printf("Error in rclc_subscription_init_default: %d\n", rc);
        assert(false);
    }
    // Message object to receive publisher data
    std_msgs__msg__UInt8 msg;
    // Add subscription to the executor
    rc = rclc_executor_add_subscription(
        &executor, &subscriber, &msg,
        subscription_callback, ON_NEW_DATA);
    if (RCL_RET_OK != rc) {
        printf("Error in rclc_executor_add_subscription: %d\n", rc);
        assert(false);
    }

    rclc_timer_init_default2(
        &timer,
        &support,
        RCL_MS_TO_NS(1000),
        timer_callback, true);
    rclc_executor_add_timer(&executor, &timer);

    // Notify mircro ros tasks
    xTaskNotifyGive(microRosPublishTask->taskHandler);

    while (1) {
        // Spin executor to receive messages
        rclc_executor_spin_period(&executor,  RCL_MS_TO_NS(100));
    }
}

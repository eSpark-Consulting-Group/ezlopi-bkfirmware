#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_service_broadcast.h"
#include "ezlopi_core_ezlopi_broadcast.h"
#include "ezlopi_core_processes.h"

static QueueHandle_t __broadcast_queue = NULL;

static void __broadcast_process(void* pv);
static int ezlopi_service_broadcast_send_to_queue(cJSON* cj_broadcast_data);

void ezlopi_service_broadcast_init(void)
{
    // __broadcast_queue = xQueueCreate(sizeof(char*), 10);
    __broadcast_queue = xQueueCreate(10, sizeof(cJSON*));
    if (__broadcast_queue)
    {
        TaskHandle_t ezlopi_service_broadcast_task_handle = NULL;
        ezlopi_core_ezlopi_broadcast_methods_set_queue(ezlopi_service_broadcast_send_to_queue);
        xTaskCreate(__broadcast_process, "broadcast-service", EZLOPI_SERVICE_BROADCAST_TASK_DEPTH, NULL, 2, &ezlopi_service_broadcast_task_handle);
        ezlopi_core_process_set_process_info(ENUM_EZLOPI_SERVICE_BROADCAST_TASK, &ezlopi_service_broadcast_task_handle, EZLOPI_SERVICE_BROADCAST_TASK_DEPTH);
    }
}

static void __broadcast_process(void* pv)
{
    static cJSON* cj_data = NULL;
    static uint32_t broadcast_wait_start = 0;

    while(1)
    {
        if (cj_data)
        {
            if ((xTaskGetTickCount() - broadcast_wait_start) > 1000 / portTICK_RATE_MS)
            {
                ezlopi_core_ezlopi_broadcast_cjson(cj_data);
                cJSON_Delete( cj_data);
                cj_data = NULL;
            }
        }
        else
        {
            if (pdTRUE == xQueueReceive(__broadcast_queue, &cj_data, 0))
            {
                broadcast_wait_start = xTaskGetTickCount();
            }
        }
        vTaskDelay(1);
    }
}

static int ezlopi_service_broadcast_send_to_queue(cJSON* cj_broadcast_data)
{
    int ret = 0;

    if (__broadcast_queue && cj_broadcast_data)
    {
        if (xQueueIsQueueFullFromISR(__broadcast_queue))
        {
            cJSON* cj_tmp_data = NULL;
            if (pdTRUE == xQueueReceive(__broadcast_queue, &cj_tmp_data, 5 / portTICK_PERIOD_MS))
            {
                if (cj_tmp_data)
                {
                    cJSON_Delete( cj_tmp_data);
                }
            }
        }
        else
        {
            TRACE_S(" ----- Adding to broadcast queue -----");
        }

        cJSON* cj_data = cj_broadcast_data;
        if (pdTRUE == xQueueSend(__broadcast_queue, &cj_data, 500/ portTICK_PERIOD_MS))
        {
            ret = 1;
        }
        else
        {
            TRACE_D(" ----- Failed adding to queue -----");
        }
    }

    return ret;
}
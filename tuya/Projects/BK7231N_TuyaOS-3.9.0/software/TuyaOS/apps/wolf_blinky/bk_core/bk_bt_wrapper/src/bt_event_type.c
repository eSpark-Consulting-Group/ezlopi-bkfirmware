#include "bt_event_type.h"
#include "tal_log.h"

void bt_event_print(const void *item) 
{
    const esp_bt_event_t *event = (const esp_bt_event_t *)item;
    TAL_PR_INFO("gatt_if=%d, Num of handles=%d" ,event->gatt_if, event->num_handles);
}
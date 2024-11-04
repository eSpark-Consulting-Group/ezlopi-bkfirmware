// event.h

#ifndef EVENT_H
#define EVENT_H

typedef struct {
    int gatt_if;
    int num_handles;
} esp_bt_event_t;

void bt_event_print(const void *item);

#endif // EVENT_H

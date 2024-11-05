#include "bk_bt_hal.h"
#include "tal_log.h"

static TKL_BLE_GATTS_PARAMS_T hal_ble_gatt_service = {0};
static TKL_BLE_SERVICE_PARAMS_T *hal_ptkl_ble_service = NULL;
static TKL_BLE_CHAR_PARAMS_T *hal_ptkl_ble_service_char = NULL;

typedef struct bk_bt_hal_service {
    TKL_BLE_SERVICE_PARAMS_T *hal_ptkl_ble_service;
    struct bk_bt_hal_service* next;
} bk_bt_hal_service_t;

typedef struct bk_bt_hal_characteristic {
    TKL_BLE_CHAR_PARAMS_T *hal_char_params;
    struct bk_bt_hal_characteristic* next;
} bk_bt_hal_characteristic_t;

static bk_bt_hal_service_t* __bk_ble_hal_service = NULL;
static bk_bt_hal_characteristic_t* __bk_ble_hal_characteristic = NULL;

// Function to create and initialize a BLE service with configurable parameters
void bk_bt_hal_create_service(const bk_bt_service_config_t* config)
{
    if (config == NULL) {
        TAL_PR_ERR("Invalid configuration provided for BLE service");
        return;
    }

    // Allocate memory for the new service
    bk_bt_hal_service_t* new_service = (bk_bt_hal_service_t*)malloc(sizeof(bk_bt_hal_service_t));
    if (new_service == NULL) {
        TAL_PR_ERR("Memory allocation for BLE service failed");
        return;
    }

    // Allocate memory for service parameters
    new_service->hal_ptkl_ble_service = (TKL_BLE_SERVICE_PARAMS_T*)malloc(sizeof(TKL_BLE_SERVICE_PARAMS_T));
    if (new_service->hal_ptkl_ble_service == NULL) {
        TAL_PR_ERR("Memory allocation for service parameters failed");
        free(new_service);
        return;
    }

    // Initialize the service parameters with configuration
    new_service->hal_ptkl_ble_service->handle = config->handle;
    new_service->hal_ptkl_ble_service->svc_uuid = config->svc_uuid;
    new_service->hal_ptkl_ble_service->type = config->type;
    new_service->hal_ptkl_ble_service->char_num = config->num_chars;   // To be populated later
    new_service->hal_ptkl_ble_service->p_char = NULL;  // To be populated later

    new_service->next = NULL;

    // Link the new service to the list of services
    if (__bk_ble_hal_service == NULL) {
        __bk_ble_hal_service = new_service;
    } else {
        bk_bt_hal_service_t* temp = __bk_ble_hal_service;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_service;
    }

    TAL_PR_DEBUG("BLE service created and added to list with UUID: 0x%X", config->svc_uuid.uuid.uuid16);
}

// Function to create and initialize a BLE characteristic with configurable parameters
void bk_bt_hal_create_characteristic(const bk_bt_char_config_t* config)
{
    if (config == NULL) {
        TAL_PR_ERR("Invalid configuration provided for BLE characteristic");
        return;
    }

    // Allocate memory for the new characteristic node
    bk_bt_hal_characteristic_t* new_char = (bk_bt_hal_characteristic_t*)malloc(sizeof(bk_bt_hal_characteristic_t));
    if (new_char == NULL) {
        TAL_PR_ERR("Memory allocation for BLE characteristic failed");
        return;
    }

    new_char->hal_char_params = (TKL_BLE_CHAR_PARAMS_T*)malloc(sizeof(TKL_BLE_CHAR_PARAMS_T));
    if (new_char->hal_char_params == NULL) {
        TAL_PR_ERR("Memory allocation for service parameters failed");
        free(new_char);
        return;
    }

    // Initialize characteristic parameters with configuration
    new_char->hal_char_params->handle = config->handle;
    new_char->hal_char_params->char_uuid = config->char_uuid;
    new_char->hal_char_params->property = config->property;
    new_char->hal_char_params->permission = config->permission;
    new_char->hal_char_params->value_len = config->value_len;
    new_char->next = NULL;

    // Link the new characteristic to the list of characteristics
    if (__bk_ble_hal_characteristic == NULL) {
        __bk_ble_hal_characteristic = new_char;
    } else {
        bk_bt_hal_characteristic_t* temp = __bk_ble_hal_characteristic;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_char;
    }

    TAL_PR_DEBUG("BLE characteristic created and added to list with UUID: 0x%X, property: %u, permission: %u, value length: %u", 
                 config->char_uuid.uuid.uuid16, config->property, config->permission, config->value_len);
}

void* bk_bt_hal_start_services()
{
    static services_started = false;
    static service_count = 0;
    static char_count = 0;
    if(__bk_ble_hal_service)
    {
        services_started = true;

        bk_bt_hal_service_t *__head_hal_service = __bk_ble_hal_service;
        bk_bt_hal_characteristic_t *__head_hal_char = __bk_ble_hal_characteristic;

        while(__head_hal_service)
        {
            service_count++; 
            __head_hal_service = __head_hal_service->next;
        }

        while(__head_hal_char)
        {
            char_count++;
            __head_hal_char = __head_hal_char->next;
        }

        if (!hal_ptkl_ble_service) {
            hal_ptkl_ble_service = tal_malloc(sizeof(TKL_BLE_SERVICE_PARAMS_T) * service_count);
        }
        if (!hal_ptkl_ble_service_char) {
            hal_ptkl_ble_service_char = tal_malloc(sizeof(TKL_BLE_CHAR_PARAMS_T) * char_count);
        }

        /*Populating final services*/
        hal_ble_gatt_service.svc_num = service_count;
        hal_ble_gatt_service.p_service = hal_ptkl_ble_service;

        int svc_indx = 0;
        int chr_indx = 0;
        __head_hal_service = __bk_ble_hal_service;
        __head_hal_char = __bk_ble_hal_characteristic;
        while(chr_indx!=char_count)
        {
            memcpy(hal_ptkl_ble_service_char + chr_indx, __head_hal_char->hal_char_params, sizeof(TKL_BLE_CHAR_PARAMS_T));
            TAL_PR_DEBUG("CHAR CREATED WITH UUID 0x%X", __head_hal_char->hal_char_params->char_uuid.uuid.uuid16);

            __head_hal_char = __head_hal_char->next;
            chr_indx++;
        }
        // 401, 301, 302, 303

        // E3, E4
        while(svc_indx != service_count)
        {
            __head_hal_service->hal_ptkl_ble_service->p_char = hal_ptkl_ble_service_char + (svc_indx * __head_hal_service->hal_ptkl_ble_service->char_num);
            memcpy(hal_ptkl_ble_service + svc_indx, __head_hal_service->hal_ptkl_ble_service, sizeof(TKL_BLE_SERVICE_PARAMS_T));
            TAL_PR_DEBUG("SERVICE CREATED WITH UUID 0x%X", __head_hal_service->hal_ptkl_ble_service->svc_uuid.uuid.uuid16);

            // while(chr_indx != __head_hal_service->hal_ptkl_ble_service->char_num)
            // {
            //     memcpy(hal_ptkl_ble_service_char + chr_indx, __head_hal_char->hal_char_params, sizeof(TKL_BLE_CHAR_PARAMS_T));
            //     __head_hal_char = __head_hal_char->next;
            //     chr_indx++;
            // }
            // chr_indx = 0;

            __head_hal_service = __head_hal_service->next;
            svc_indx++;
        }

    }
    return &hal_ble_gatt_service;
}

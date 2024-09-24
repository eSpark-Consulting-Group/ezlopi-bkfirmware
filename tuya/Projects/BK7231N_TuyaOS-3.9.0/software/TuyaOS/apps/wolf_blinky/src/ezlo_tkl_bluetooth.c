#include "ezlo_tkl_bluetooth.h"

#include "tkl_bluetooth.h"
//#include "tuya_ble_hs_init.h"
// #include "ble_svc_gap.h"
// #include "ble_svc_gatt.h"
// #include "ble_gap.h"
// #include "ble_hs_log.h"
#include "tal_log.h"
#include "tkl_mutex.h"
// Standard Files
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "tal_system.h"
#include "tkl_hci.h"
#include "tal_memory.h"

/*** @client. */
struct ble_gatt_error {
    uint16_t status;
    uint16_t att_handle;
};

/** Generic UUID type, to be used only as a pointer */
typedef struct {
    /** Type of the UUID */
    uint8_t type;
} ble_uuid_t;

/** 16-bit UUID */
typedef struct {
    ble_uuid_t u;
    uint16_t value;
} ble_uuid16_t;

/** 32-bit UUID */
typedef struct {
    ble_uuid_t u;
    uint32_t value;
} ble_uuid32_t;

/** 128-bit UUID */
typedef struct {
    ble_uuid_t u;
    uint8_t value[16];
} ble_uuid128_t;

/** Universal UUID type, to be used for any-UUID static allocation */
typedef union {
    ble_uuid_t u;
    ble_uuid16_t u16;
    ble_uuid32_t u32;
    ble_uuid128_t u128;
} ble_uuid_any_t;

struct ble_gatt_svc {
    uint16_t start_handle;
    uint16_t end_handle;
    ble_uuid_any_t uuid;
};

struct ble_gatt_dsc {
    uint16_t handle;
    ble_uuid_any_t uuid;
};

struct ble_gatt_chr {
    uint16_t def_handle;
    uint16_t val_handle;
    uint8_t properties;
    ble_uuid_any_t uuid;
};
#define TY_HS_BLE_ROLE_CENTRAL 1

#if (TY_HS_BLE_ROLE_CENTRAL)
typedef struct {
    uint8_t role; /*< Refer to @ TKL_BLE_ROLE_SERVER and TKL_BLE_ROLE_CLIENT */
    TKL_BLE_GATT_SVC_DISC_TYPE_T service;
    TKL_BLE_GATT_CHAR_DISC_TYPE_T char_disc;
    TKL_BLE_GATT_DESC_DISC_TYPE_T desc_disc;
} TKL_BLUETOOTH_CLIENT_PARAMS_T;
#endif

#if (TY_HS_BLE_ROLE_CENTRAL)
static TKL_BLUETOOTH_CLIENT_PARAMS_T tuya_ble_client;
#endif

static TKL_BLE_GAP_EVT_FUNC_CB ezlo_tkl_bluetooth_gap_callback;
static TKL_BLE_GATT_EVT_FUNC_CB ezlo_tkl_bluetooth_gatt_callback;

typedef int ble_gatt_disc_svc_fn(uint16_t conn_handle,
                                 const struct ble_gatt_error *error,
                                 const struct ble_gatt_svc *service,
                                 void *arg);

typedef int ble_gatt_chr_fn(uint16_t conn_handle,
                            const struct ble_gatt_error *error,
                            const struct ble_gatt_chr *chr, void *arg);

typedef int ble_gatt_dsc_fn(uint16_t conn_handle,
                            const struct ble_gatt_error *error,
                            uint16_t chr_val_handle,
                            const struct ble_gatt_dsc *dsc,
                            void *arg);

extern int ble_gattc_disc_all_svcs(uint16_t conn_handle,
                            ble_gatt_disc_svc_fn *cb, void *cb_arg);

extern int ble_gattc_disc_all_chrs(uint16_t conn_handle, uint16_t start_handle,
                            uint16_t end_handle, ble_gatt_chr_fn *cb,
                            void *cb_arg);

extern int ble_gattc_disc_all_dscs(uint16_t conn_handle, uint16_t start_handle,
                            uint16_t end_handle,
                            ble_gatt_dsc_fn *cb, void *cb_arg);

extern int ble_gattc_write_no_rsp_flat(uint16_t conn_handle, uint16_t attr_handle,
                                const void *data, uint16_t data_len);

#if (TY_HS_BLE_ROLE_CENTRAL)
#define BLE_GATT_SVC_UUID16                             0x1801
#define BLE_GATT_DSC_CLT_CFG_UUID16                     0x2902
#define BLE_HS_EAGAIN               1
#define BLE_HS_EALREADY             2
#define BLE_HS_EINVAL               3
#define BLE_HS_EMSGSIZE             4
#define BLE_HS_ENOENT               5
#define BLE_HS_ENOMEM               6
#define BLE_HS_ENOTCONN             7
#define BLE_HS_ENOTSUP              8
#define BLE_HS_EAPP                 9
#define BLE_HS_EBADDATA             10
#define BLE_HS_EOS                  11
#define BLE_HS_ECONTROLLER          12
#define BLE_HS_ETIMEOUT             13
#define BLE_HS_EDONE                14
#define BLE_HS_EBUSY                15
#define BLE_HS_EREJECT              16
#define BLE_HS_EUNKNOWN             17
#define BLE_HS_EROLE                18
#define BLE_HS_ETIMEOUT_HCI         19
#define BLE_HS_ENOMEM_EVT           20
#define BLE_HS_ENOADDR              21
#define BLE_HS_ENOTSYNCED           22
#define BLE_HS_EAUTHEN              23
#define BLE_HS_EAUTHOR              24
#define BLE_HS_EENCRYPT             25
#define BLE_HS_EENCRYPT_KEY_SZ      26
#define BLE_HS_ESTORE_CAP           27
#define BLE_HS_ESTORE_FAIL          28
#define BLE_HS_EPREEMPTED           29
#define BLE_HS_EDISABLED            30
#define BLE_HS_ESTALLED             31
static int tuya_ble_svc_disc_callback(uint16_t conn_handle, const struct ble_gatt_error *error,
                                      const struct ble_gatt_svc *service, void *arg)
{
    TKL_BLE_GATT_PARAMS_EVT_T gatt_event;
    TKL_BLE_GATT_SVC_DISC_TYPE_T *p_service = &tuya_ble_client.service;
    uint8_t svc_index = p_service->svc_num;

    gatt_event.type = TKL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY;
    gatt_event.conn_handle = conn_handle;
    switch (error->status) {
    case 0: {
        if (service->uuid.u.type == TKL_BLE_UUID_TYPE_16) {
            p_service->services[svc_index].uuid.uuid_type = TKL_BLE_UUID_TYPE_16;
            p_service->services[svc_index].uuid.uuid.uuid16 = service->uuid.u16.value;
        } else if (service->uuid.u.type == TKL_BLE_UUID_TYPE_32) {
            p_service->services[svc_index].uuid.uuid_type = TKL_BLE_UUID_TYPE_32;
            p_service->services[svc_index].uuid.uuid.uuid32 = service->uuid.u32.value;
        } else if (service->uuid.u.type == TKL_BLE_UUID_TYPE_128) {
            p_service->services[svc_index].uuid.uuid_type = TKL_BLE_UUID_TYPE_128;
            memcpy(p_service->services[svc_index].uuid.uuid.uuid128, service->uuid.u128.value, 16);
        }

        p_service->services[svc_index].start_handle = service->start_handle;
        p_service->services[svc_index].end_handle = service->end_handle;

        p_service->svc_num++;
        TAL_PR_DEBUG("Discovery Service, Service(0x%02x)", service->uuid.u16.value);
        TAL_PR_DEBUG("Start Handle = 0x%04x, End Handle = 0x%04x", service->start_handle, service->end_handle);
        return 0;
    }; // Need to return

    case BLE_HS_EDONE:
        memcpy(&gatt_event.gatt_event.svc_disc, p_service, sizeof(TKL_BLE_GATT_SVC_DISC_TYPE_T));
        memset(p_service, 0, sizeof(TKL_BLE_GATT_SVC_DISC_TYPE_T));

        gatt_event.result = OPRT_OK;
        /* All services discovered; start discovering characteristics. */
        TAL_PR_INFO("Finish Discovery Service, Success");
        break;

    default:
        memset(p_service, 0, sizeof(TKL_BLE_GATT_SVC_DISC_TYPE_T));
        gatt_event.result = OPRT_OS_ADAPTER_BLE_SVC_DISC_FAILED;

        TAL_PR_INFO("Finish Discovery Service, Fail, status(%d)", error->status);
        break;
    }

    if (ezlo_tkl_bluetooth_gatt_callback) {
        ezlo_tkl_bluetooth_gatt_callback(&gatt_event);
    }
    return OPRT_OK;
}

static int tuya_ble_chr_disc_callback(uint16_t conn_handle, const struct ble_gatt_error *error,
                                      const struct ble_gatt_chr *chr, void *arg)
{
    TKL_BLE_GATT_PARAMS_EVT_T gatt_event;
    TKL_BLE_GATT_CHAR_DISC_TYPE_T *p_char = &tuya_ble_client.char_disc;
    uint8_t char_index = p_char->char_num;

    gatt_event.type = TKL_BLE_GATT_EVT_CHAR_DISCOVERY;
    gatt_event.conn_handle = conn_handle;

    switch (error->status) {
    case 0: {
        if (chr->uuid.u.type == TKL_BLE_UUID_TYPE_16) {
            p_char->characteristics[char_index].uuid.uuid_type = TKL_BLE_UUID_TYPE_16;
            p_char->characteristics[char_index].uuid.uuid.uuid16 = chr->uuid.u16.value;
        } else if (chr->uuid.u.type == TKL_BLE_UUID_TYPE_32) {
            p_char->characteristics[char_index].uuid.uuid_type = TKL_BLE_UUID_TYPE_32;
            p_char->characteristics[char_index].uuid.uuid.uuid32 = chr->uuid.u32.value;
        } else if (chr->uuid.u.type == TKL_BLE_UUID_TYPE_128) {
            p_char->characteristics[char_index].uuid.uuid_type = TKL_BLE_UUID_TYPE_128;
            memcpy(p_char->characteristics[char_index].uuid.uuid.uuid128, chr->uuid.u128.value, 16);
        }

        p_char->characteristics[char_index].handle = chr->val_handle;

        TAL_PR_DEBUG("Discovery Characteristics, Value Handle = 0x%04x, UUID Value = 0x%04x", chr->val_handle,
                         chr->uuid.u16.value);
        return 0;
    };

    case BLE_HS_EDONE:
        /* All services discovered; start discovering characteristics. */
        memcpy(&gatt_event.gatt_event.char_disc, p_char, sizeof(TKL_BLE_GATT_SVC_DISC_TYPE_T));
        gatt_event.result = OPRT_OK;
        TAL_PR_INFO("Finish Discovery Characteristics");
        break;

    default:
        gatt_event.result = OPRT_OS_ADAPTER_BLE_CHAR_DISC_FAILED;
        break;
    }

    if (ezlo_tkl_bluetooth_gatt_callback) {
        ezlo_tkl_bluetooth_gatt_callback(&gatt_event);
    }
    return OPRT_OK;
}

static int tuya_ble_desc_disc_callback(uint16_t conn_handle, const struct ble_gatt_error *error,
                                       uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc, void *arg)
{
    TKL_BLE_GATT_PARAMS_EVT_T gatt_event;

    gatt_event.type = TKL_BLE_GATT_EVT_CHAR_DESC_DISCOVERY;
    gatt_event.conn_handle = conn_handle;

    switch (error->status) {
    case 0:
        if (dsc->uuid.u16.value == BLE_GATT_DSC_CLT_CFG_UUID16) {
            tuya_ble_client.desc_disc.cccd_handle = dsc->handle;
        }
        TAL_PR_INFO("Discovery Characteristics Descriptor Handle = 0x%04x", dsc->handle);
        return 0;

    case BLE_HS_EDONE:
        /* All descriptors in this characteristic discovered; start discovering
         * descriptors in the next characteristic.
         */
        gatt_event.gatt_event.desc_disc.cccd_handle = tuya_ble_client.desc_disc.cccd_handle;
        gatt_event.result = OPRT_OK;
        TAL_PR_INFO("Finish Discovery Characteristics Descriptor (0x2902)");
        break;

    default:
        /* Error; abort discovery. */
        gatt_event.result = OPRT_OS_ADAPTER_BLE_DESC_DISC_FAILED;
        break;
    }

    if (ezlo_tkl_bluetooth_gatt_callback) {
        ezlo_tkl_bluetooth_gatt_callback(&gatt_event);
    }
    return OPRT_OK;
}
#endif

/**
 * @brief   [Ble Central] Will Discovery All Descriptor of Characteristic
 * @param   [in] conn_handle    Connection handle.
 * @param   [in] conn_handle    Connection handle.
 *          [in] start_handle   Handle of start
 *          [in] end_handle     Handle of End
 *          [in] p_uuid         descriptor uuid
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET ezlo_tkl_ble_gattc_char_desc_discovery(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    if (!ble_hs_is_enabled()) {
        TAL_PR_INFO("bt_stack close,bt operation invalid.\n");
        return OPRT_OK;
    }

    int rc = OPRT_COM_ERROR;

    TAL_PR_DEBUG("Discovery All Descriptors, Start Handle = 0x%04x, End Handle = 0x%04x", start_handle, end_handle);
    rc = ble_gattc_disc_all_dscs(conn_handle, start_handle, end_handle, tuya_ble_desc_disc_callback, NULL);
    if (rc != 0) {
        TAL_PR_DEBUG("Discovery Char desc fail(%d)", rc);
        return OPRT_OS_ADAPTER_BLE_DESC_DISC_FAILED;
    }

    return OPRT_OK;
}

/**
 * @brief   [Ble Central] Write Data without Response
 * @param   [in] conn_handle    Connection handle.
 * @param   [in] char_handle    Attribute handle.
 *          [in] p_data         Write Values
 *          [in] length         Value Length
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET ezlo_tkl_ble_gattc_write_without_rsp(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data,
                                            uint16_t length)
{
    if (!ble_hs_is_enabled()) {
        TAL_PR_INFO("bt_stack close,bt operation invalid.\n");
        return OPRT_OK;
    }

    int rc;

    rc = ble_gattc_write_no_rsp_flat(conn_handle, char_handle, p_data, length);
    if (rc != 0) {
        TAL_PR_ERR("Error: Failed to write characteristic; rc=%d\n", rc);
        return OPRT_OS_ADAPTER_BLE_WRITE_FAILED;
    }

    return 0;
}

/**
 * @brief   [Ble Central] Will Discovery All Characteristic
 * @param   [in] conn_handle    Connection handle.
 *          [in] start_handle   Handle of start
 *          [in] end_handle     Handle of End
 *          [in] p_uuid         char uuid
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET ezlo_tkl_ble_gattc_all_char_discovery(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    if (!ble_hs_is_enabled()) {
        TAL_PR_INFO("bt_stack close,bt operation invalid.\n");
        return OPRT_OK;
    }

    int rc = OPRT_COM_ERROR;

    TAL_PR_DEBUG("Discovery All Char, Start Handle = 0x%04x, End Handle = 0x%04x", start_handle, end_handle);
    rc = ble_gattc_disc_all_chrs(conn_handle, start_handle, end_handle, tuya_ble_chr_disc_callback, NULL);
    if (rc != 0) {
        TAL_PR_DEBUG("Discovery Char fail(%d)", rc);
        return OPRT_OS_ADAPTER_BLE_CHAR_DISC_FAILED;
    }

    return OPRT_OK;
}

/**
 * @brief   [Ble Central] Will Discovery All Service
 * @param   [in] conn_handle    Connection handle.
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET ezlo_tkl_ble_gattc_all_service_discovery(uint16_t conn_handle)
{
    if (!ble_hs_is_enabled()) {
        TAL_PR_INFO("bt_stack close,bt operation invalid.\n");
        return OPRT_OK;
    }

    int rc = OPRT_COM_ERROR;

    rc = ble_gattc_disc_all_svcs(conn_handle, tuya_ble_svc_disc_callback, NULL);
    if (rc != 0) {
        return OPRT_OS_ADAPTER_BLE_SVC_DISC_FAILED;
    }

    return OPRT_OK;
}


/**
 * @brief   Register GAP Event Callback
 * @param   TKL_BLE_GAP_EVT_FUNC_CB Refer to @TKL_BLE_GAP_EVT_FUNC_CB
 * @return  SUCCESS              Register successfully.
 *          ERROR
 * */
OPERATE_RET ezlo_tkl_ble_gap_callback_register(const TKL_BLE_GAP_EVT_FUNC_CB gap_evt)
{
    ezlo_tkl_bluetooth_gap_callback = gap_evt;

    // tuya_ble_hs_cfg.reset_cb = tuya_ble_host_stack_reset_callback;
    // tuya_ble_hs_cfg.sync_cb = tuya_ble_host_stack_sync_callback;
    return OPRT_OK;
}

/**
 * @brief   Register GATT Event Callback
 * @param   TKL_BLE_GATT_EVT_FUNC_CB Refer to @TKL_BLE_GATT_EVT_FUNC_CB
 * @return  SUCCESS              Register successfully.
 *          ERROR
 * */
OPERATE_RET ezlo_tkl_ble_gatt_callback_register(const TKL_BLE_GATT_EVT_FUNC_CB gatt_evt)
{
    // Register Gatt Callback
    ezlo_tkl_bluetooth_gatt_callback = gatt_evt;
    return OPRT_OK;
}
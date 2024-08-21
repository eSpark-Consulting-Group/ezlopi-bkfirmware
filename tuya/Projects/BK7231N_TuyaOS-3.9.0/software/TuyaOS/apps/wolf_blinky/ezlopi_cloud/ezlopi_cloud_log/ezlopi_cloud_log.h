

#ifndef _EZLOPI_CLOUD_LOG_H_
#define _EZLOPI_CLOUD_LOG_H_

#include "sdkconfig.h"

#ifdef CONFIG_EZPI_UTIL_TRACE_EN


#include "cJSON.h"


void ezlopi_hub_cloud_log_set(cJSON* cj_request, cJSON* cj_response);

void ezlopi_hub_serial_log_set(cJSON* cj_request, cJSON* cj_response);
void ezlopi_hub_serial_log_set_updater(cJSON* cj_request, cJSON* cj_response);

#endif  // CONFIG_EZPI_UTIL_TRACE_EN

#endif // _EZLOPI_CLOUD_LOG_H_


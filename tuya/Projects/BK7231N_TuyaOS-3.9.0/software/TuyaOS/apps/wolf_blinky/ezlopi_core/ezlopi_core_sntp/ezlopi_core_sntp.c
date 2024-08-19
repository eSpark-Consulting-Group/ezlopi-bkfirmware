
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "tal_time_service.h"

#include "sdkconfig.h"

#include "bk_sntp.h"
#include "zones.h"

#include "ezlopi_util_trace.h"
#include "ezlopi_core_sntp.h"
#include "ezlopi_core_nvs.h"
#include "ezlopi_core_wifi.h"

static time_t start_time = 0;

// static void sntp_sync_time_call_back(struct timeval* tv)
// {

//     char strftime_buf[64];
//     struct tm timeinfo;

//     sntp_set_sync_interval(60 * 60 * 1000); // Sync every hour
//     TRACE_I("Notification of a time synchronization event");

//     time_t now;
//     time(&now);
//     if (0 == start_time)
//     {
//         start_time = now;
//     }

//     localtime_r(&now, &timeinfo);

//     strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//     TRACE_I("Time now[%ld]: %s", now, strftime_buf);
// }

void EZPI_CORE_sntp_init(void)
{
    // int retry = 0;
    // const int retry_count = 10;

    TRACE_I("Initializing SNTP");

    // esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    // esp_sntp_setservername(0, "pool.ntp.org");
    // esp_sntp_setservername(1, "ntp-b.nist.gov");
    // esp_sntp_setservername(2, "ntp-wwv.nist.gov");
    // sntp_set_time_sync_notification_cb(sntp_sync_time_call_back);
    

    // sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);

    // sntp_set_sync_interval(15 * 1000);
    // esp_sntp_init();

    // while ((sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) && (++retry < retry_count))
    // {
    //     TRACE_I("Waiting for system time to be set... (%d/%d)", retry, retry_count);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    // }
    bk_sntp_init();
}

int EZPI_CORE_sntp_set_location(const char* location)
{
    int ret = 1;
    if (location)
    {
        if (EZPI_CORE_nvs_write_time_location(location, strlen(location)))
        {

            const char* posix_str = micro_tz_db_get_posix_str(location);

            if (NULL == posix_str)
            {
                TRACE_I("%s is not a known timezone!\n", location);
                ret = 0;
            }
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        ret = 0;
    }
    return ret;
}

char* EZPI_CORE_sntp_get_location(void)
{
    return EZPI_CORE_nvs_read_time_location();
}

#if 0
void EZPI_CORE_sntp_get_local_time(char* time_buf, uint32_t buf_len)
{
    if (time_buf && buf_len)
    {
        char* location = EZPI_CORE_nvs_read_time_location();
        const char* posix_str = (location) ? micro_tz_db_get_posix_str(location) : NULL;

        if (!posix_str)
        {
            TRACE_W("Unknown time zone location or invalid location, setting GMT0!!");
            posix_str = "GMT0";
        }

        if (setenv("TZ", posix_str, 1) != 0)
        {
            TRACE_E("Error setting time zone: %s", posix_str);
            return;
        }
        else
        {
            tzset();

            TRACE_I("Timezone set: %s", posix_str);

            time_t now;
            struct tm timeinfo;

            time(&now);
            localtime_r(&now, &timeinfo);

            if (strftime(time_buf, buf_len, "%Y-%m-%dT%H:%M:%S%z", &timeinfo))
            {
                TRACE_I("The current date/time int %s is: %s", location ? location : posix_str, time_buf);
            }
            else
            {
                TRACE_E("Error - strftime");
            }
        }

        if (location)
        {
            ezlopi_free(__FUNCTION__, location);
        }
    }
}
#endif
void EZPI_CORE_sntp_get_local_time(char* time_buf, uint32_t buf_len)
{
    if (time_buf && buf_len)
    {
        char* location = EZPI_CORE_nvs_read_time_location();
        const char* posix_str = (location) ? micro_tz_db_get_posix_str(location) : NULL;

        if (!posix_str)
        {
            TRACE_W("Unknown time zone location or invalid location, setting GMT0!!");
            posix_str = "GMT-5";
        }

        if (setenv("TZ", posix_str, 1) != 0)
        {
            TRACE_E("Error setting time zone: %s", posix_str);
            return;
        }
        else
        {
            tzset();

            TRACE_I("Timezone set: %s", posix_str);

            // Initialize the POSIX_TM_S structure
            POSIX_TM_S tm;

            // Get the current time using tal_time_get
            if (tal_time_get(&tm) == 0) // Check if the time was retrieved successfully
            {
                // Convert POSIX_TM_S to struct tm
                struct tm timeinfo;
                timeinfo.tm_sec = tm.tm_sec;
                timeinfo.tm_min = tm.tm_min;
                timeinfo.tm_hour = tm.tm_hour;
                timeinfo.tm_mday = tm.tm_mday;
                timeinfo.tm_mon = tm.tm_mon;
                timeinfo.tm_year = tm.tm_year;
                timeinfo.tm_wday = tm.tm_wday;
                timeinfo.tm_yday = -1; // tm_yday is typically unused in this context
                timeinfo.tm_isdst = -1; // Daylight saving time flag

                // Format the time into ISO 8601 format using strftime
                if (strftime(time_buf, buf_len, "%Y-%m-%dT%H:%M:%S%z", &timeinfo))
                {
                    TRACE_I("The current date/time in %s is: %s", location ? location : posix_str, time_buf);
                }
                else
                {
                    TRACE_E("Error - strftime");
                }
            }
            else
            {
                TRACE_E("Error retrieving time");
            }
        }

        if (location)
        {
            ezlopi_free(__FUNCTION__, location);
        }
    }
}

#if 0
void EZPI_CORE_sntp_get_up_time(char* up_time_buf, uint32_t buf_len)
{
    if (up_time_buf && buf_len)
    {
        struct tm timeinfo;
        localtime_r(&start_time, &timeinfo);
        strftime(up_time_buf, buf_len, "%c", &timeinfo);
    }
}
#endif
void EZPI_CORE_sntp_get_up_time(char* up_time_buf, uint32_t buf_len)
{
    if (up_time_buf && buf_len)
    {
        // Initialize the POSIX_TM_S structure
        POSIX_TM_S tm;
        
        // Retrieve the current time using tal_time_get
        if (tal_time_get(&tm) == 0) // Check if the time was retrieved successfully
        {
            // Convert POSIX_TM_S to struct tm
            struct tm timeinfo;
            timeinfo.tm_sec = tm.tm_sec;
            timeinfo.tm_min = tm.tm_min;
            timeinfo.tm_hour = tm.tm_hour;
            timeinfo.tm_mday = tm.tm_mday;
            timeinfo.tm_mon = tm.tm_mon;
            timeinfo.tm_year = tm.tm_year;
            timeinfo.tm_wday = tm.tm_wday;
            
            // Format the time into a string using strftime
            strftime(up_time_buf, buf_len, "%c", &timeinfo);
        }
    }
}

void EZPI_CORE_sntp_epoch_to_iso8601(char* time_buf, uint32_t buf_len, time_t t)
{
    if (time_buf && buf_len)
    {
        // Initialize the POSIX_TM_S structure
        POSIX_TM_S tm;

        // Mock conversion for demonstration
        // Normally you would use a method to set tm based on `t`
        // Since `tal_time_get` gets the current time, simulate conversion manually if needed
        
        // Initialize POSIX_TM_S for the given time `t`
        struct tm *timeinfo = localtime(&t); // Convert the given epoch time to local time
        tm.tm_sec = timeinfo->tm_sec;
        tm.tm_min = timeinfo->tm_min;
        tm.tm_hour = timeinfo->tm_hour;
        tm.tm_mday = timeinfo->tm_mday;
        tm.tm_mon = timeinfo->tm_mon;
        tm.tm_year = timeinfo->tm_year;
        tm.tm_wday = timeinfo->tm_wday;

        // Format the time into ISO 8601 format using strftime
        struct tm timeinfo_local;
        timeinfo_local.tm_sec = tm.tm_sec;
        timeinfo_local.tm_min = tm.tm_min;
        timeinfo_local.tm_hour = tm.tm_hour;
        timeinfo_local.tm_mday = tm.tm_mday;
        timeinfo_local.tm_mon = tm.tm_mon;
        timeinfo_local.tm_year = tm.tm_year;
        timeinfo_local.tm_wday = tm.tm_wday;

        if (strftime(time_buf, buf_len, "%Y-%m-%dT%H:%M:%S%z", &timeinfo_local))
        {
            // Time formatted successfully
        }
    }
}

uint64_t EZPI_CORE_sntp_get_current_time_ms(void)
{
    POSIX_TM_S tm;
    if (tal_time_get(&tm) == 0) // Check if the time was retrieved successfully
    {
        time_t now = mktime(&(struct tm){
            .tm_sec = tm.tm_sec,
            .tm_min = tm.tm_min,
            .tm_hour = tm.tm_hour,
            .tm_mday = tm.tm_mday,
            .tm_mon = tm.tm_mon,
            .tm_year = tm.tm_year,
            .tm_wday = tm.tm_wday
        });

        return (now * 1000LL);
    }
}

uint64_t EZPI_CORE_sntp_get_current_time_sec(void)
{
    POSIX_TM_S tm;
    if (tal_time_get(&tm) == 0) // Check if the time was retrieved successfully
    {
        time_t now = mktime(&(struct tm){
            .tm_sec = tm.tm_sec,
            .tm_min = tm.tm_min,
            .tm_hour = tm.tm_hour,
            .tm_mday = tm.tm_mday,
            .tm_mon = tm.tm_mon,
            .tm_year = tm.tm_year,
            .tm_wday = tm.tm_wday
        });

        return now;
    }
}


#if 0
void EZPI_CORE_sntp_epoch_to_iso8601(char* time_buf, uint32_t buf_len, time_t t)
{
    if (time_buf && buf_len)
    {
        struct tm* timeinfo;
        // timeinfo = gmtime(&t);
        timeinfo = localtime(&t);
        strftime(time_buf, buf_len, "%Y-%m-%dT%H:%M:%S%z", timeinfo);
        // TRACE_I("Build Time: %s", time_buf);
    }
}

uint64_t EZPI_CORE_sntp_get_current_time_ms(void)
{
    time_t now;
    time(&now);
    return (now * 1000LL);
}

uint64_t EZPI_CORE_sntp_get_current_time_sec(void)
{
    time_t now;
    time(&now);
    return now;
}
#endif

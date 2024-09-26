#include "bk_sntp.h"
#include "bk_core_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "time.h"
#include <tal_time_service.h>
#include "tal_log.h"

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define NTP_PORT 123
#define NTP_PACKET_SIZE 48
// #define TIMEZONE_OFFSET (5*3600) // Adjust for your timezone
#define TIMEZONE_OFFSET (0) // Adjust for your timezone

#ifndef CONFIG_BK_SNTP_DEBUG
#define TAL_PR_DEBUG
#endif

uint8_t packetBuffer[NTP_PACKET_SIZE];

void send_ntp_request(int sockfd, struct sockaddr_in *server_addr) {
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;
    packetBuffer[2] = 6;
    packetBuffer[3] = 0xEC;

    sendto(sockfd, packetBuffer, NTP_PACKET_SIZE, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
}

void receive_ntp_response(int sockfd) {
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    if (recvfrom(sockfd, packetBuffer, NTP_PACKET_SIZE, 0, (struct sockaddr *)&src_addr, &addr_len) > 0) {
        // Extract the time
        uint32_t highWord = (packetBuffer[40] << 8) | packetBuffer[41];
        uint32_t lowWord = (packetBuffer[42] << 8) | packetBuffer[43];
        uint32_t epoch = (highWord << 16) | lowWord;

        epoch -= 2208988800UL; // Convert NTP time to Unix time
        epoch += TIMEZONE_OFFSET;

        time_t current_time = (time_t)epoch;
        struct tm *time_info = gmtime(&current_time);

        POSIX_TM_S tm;
        tm.tm_sec = time_info->tm_sec;
        tm.tm_min = time_info->tm_min;
        tm.tm_hour = time_info->tm_hour;
        tm.tm_mday = time_info->tm_mday;
        tm.tm_mon = time_info->tm_mon;
        tm.tm_year = time_info->tm_year;
        tm.tm_wday = time_info->tm_wday;

        tal_time_service_init();
        // Set the system time using tal_time_set
        tal_time_set_time_zone("+05:00");
        if (tal_time_set(&tm) == 0) {
            TAL_PR_DEBUG("System time set successfully: %s", asctime(time_info));
        } else {
            TAL_PR_DEBUG("Failed to set system time\n");
        }


        TAL_PR_DEBUG("Current time from NTP server: %s", ctime(&current_time));
    } else {
        TAL_PR_ERR("Failed to receive NTP response");
    }
}

int bk_sntp_init() {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        TAL_PR_ERR("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Resolve the NTP server IP address
    server = gethostbyname(NTP_SERVER);
    if (server == NULL) {
        TAL_PR_DEBUG(stderr, "No such host: %s\n", NTP_SERVER);
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NTP_PORT);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // Send the NTP request
    send_ntp_request(sockfd, &server_addr);

    // Wait for and receive the NTP response
    receive_ntp_response(sockfd);

    // Close the socket
    close(sockfd);

    return 0;
}

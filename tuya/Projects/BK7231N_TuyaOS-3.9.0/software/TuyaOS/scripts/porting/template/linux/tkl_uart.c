/**
 * @file tkl_uart.c
 * @brief Common process - adapter the uart api default weak implement
 * @version 0.1
 * @date 2021-08-06
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

// --- BEGIN: user defines and implements ---
#include "tuya_iot_config.h"
#include "tkl_uart.h"
#include "tkl_semaphore.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#define UART_DEV_NUM 2

typedef struct {
    UINT_T port_id;
    pthread_t id;
    INT_T fd;
    TUYA_UART_IRQ_CB isr_cb;
} uart_dev_t;

STATIC uart_dev_t uart_devs[UART_DEV_NUM];
STATIC CHAR_T *COMM_UART  = NULL;
STATIC CHAR_T *DEBUG_UART = NULL;

VOID_T set_uart(CHAR_T *comm, CHAR_T *debug)
{
    SIZE_T len = strlen(comm) + 1;
    COMM_UART  = (CHAR_T *)malloc(len);
    strcpy(COMM_UART, comm);

    len        = strlen(debug) + 1;
    DEBUG_UART = (CHAR_T *)malloc(len);
    strcpy(DEBUG_UART, debug);
}

VOID_T uart_printf(CONST CHAR_T *str)
{
    STATIC INT_T fd = 0;

    if (NULL == DEBUG_UART) {
        return;
    }

    if (0 == fd) {
        fd = open(DEBUG_UART, O_RDWR); // debug
    }

    if (0 > fd) {
        printf("Open debug uart fail!!!");
        return;
    }

    while (*str) {
        write(fd, str, 1);
        str++;
    }
}

STATIC VOID_T *uart_dev_irq_handler(VOID_T *parameter)
{
    uart_dev_t *uart_dev = (uart_dev_t *)parameter;

    for (;;) {
        pthread_testcancel();

        fd_set readfd;
        FD_ZERO(&readfd);
        FD_SET(uart_dev->fd, &readfd);
        if (select(uart_dev->fd + 1, &readfd, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(uart_dev->fd, &readfd)) {
                if (uart_dev->isr_cb) {
                    uart_dev->isr_cb(uart_dev->port_id);
                }
            }
        }
    }

    return NULL;
}
// --- END: user defines and implements ---

/**
 * @brief uart init
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] cfg: uart config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_init(UINT_T port_id, TUYA_UART_BASE_CFG_T *cfg)
{
    // --- BEGIN: user implements ---
    if (port_id >= UART_DEV_NUM) {
        return OPRT_INVALID_PARM;
    }

    struct termios term_orig;
    uart_dev_t *uart_dev = &uart_devs[port_id];
    uart_dev->port_id    = port_id;
    if (0 == port_id) {
        uart_dev->fd = open(COMM_UART, O_RDWR | O_NDELAY | O_NOCTTY); // comm
    } else if (1 == port_id) {
        uart_dev->fd = open(DEBUG_UART, O_RDWR); // debug
    }

    if (0 > uart_dev->fd) {
        printf("Open UART%d fail!!!", port_id);
        return OPRT_COM_ERROR;
    }
    tcgetattr(uart_dev->fd, &term_orig);

    if (cfg->baudrate == 9600) {
        cfsetispeed(&term_orig, B9600);
        cfsetospeed(&term_orig, B9600);
    } else {
        cfsetispeed(&term_orig, B115200);
        cfsetospeed(&term_orig, B115200);
    }

    term_orig.c_iflag &= ~(BRKINT | ICRNL | IGNCR | ISTRIP | IXON);
    term_orig.c_oflag &= ~OPOST;
    term_orig.c_cflag |= CLOCAL | CREAD;
    term_orig.c_cflag &= ~CSIZE;
    term_orig.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    switch (cfg->databits) {
    case TUYA_UART_DATA_LEN_5BIT:
        term_orig.c_cflag |= CS5;
        break;
    case TUYA_UART_DATA_LEN_6BIT:
        term_orig.c_cflag |= CS6;
        break;
    case TUYA_UART_DATA_LEN_7BIT:
        term_orig.c_cflag |= CS7;
        break;
    case TUYA_UART_DATA_LEN_8BIT:
    default:
        term_orig.c_cflag |= CS8;
        break;
    }

    switch (cfg->stopbits) {
    case TUYA_UART_STOP_LEN_1BIT:
        term_orig.c_cflag &= ~CSTOPB;
        break;
    case TUYA_UART_STOP_LEN_2BIT:
        term_orig.c_cflag |= CSTOPB;
        break;
    default:
        term_orig.c_cflag &= ~CSTOPB;
        break;
    }

    switch (cfg->parity) {
    case TUYA_UART_PARITY_TYPE_ODD:
        term_orig.c_cflag |= PARENB;
        term_orig.c_cflag |= PARODD;
        term_orig.c_iflag |= INPCK;
        break;
    case TUYA_UART_PARITY_TYPE_EVEN:
        term_orig.c_cflag |= PARENB;
        term_orig.c_cflag &= ~PARODD;
        term_orig.c_iflag |= INPCK;
        break;
    case TUYA_UART_PARITY_TYPE_NONE:
    default:
        term_orig.c_cflag &= ~PARENB;
        term_orig.c_iflag &= ~(INPCK | ICRNL | IGNCR);
        term_orig.c_lflag &= ~ICANON;
        break;
    }
    tcsetattr(uart_dev->fd, TCSANOW, &term_orig);

    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief uart deinit
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_deinit(UINT_T port_id)
{
    // --- BEGIN: user implements ---
    if (port_id >= UART_DEV_NUM) {
        return OPRT_INVALID_PARM;
    }
    close(uart_devs[port_id].fd);
    uart_devs[port_id].fd      = -1;
    uart_devs[port_id].port_id = UART_DEV_NUM;
    uart_devs[port_id].isr_cb  = NULL;
    pthread_cancel(uart_devs[port_id].id);
    pthread_join(uart_devs[port_id].id, NULL);

    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief uart write data
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] data: write buff
 * @param[in] len:  buff len
 *
 * @return return > 0: number of data written; return <= 0: write errror
 */
INT_T tkl_uart_write(UINT_T port_id, VOID_T *buff, UINT16_T len)
{
    // --- BEGIN: user implements ---
    if (port_id >= UART_DEV_NUM) {
        return OPRT_INVALID_PARM;
    }

    return write(uart_devs[port_id].fd, buff, len);
    // --- END: user implements ---
}

/**
 * @brief uart read data
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[out] data: read data
 * @param[in] len:  buff len
 *
 * @return return >= 0: number of data read; return < 0: read errror
 */
INT_T tkl_uart_read(UINT_T port_id, VOID_T *buff, UINT16_T len)
{
    // --- BEGIN: user implements ---
    if (port_id >= UART_DEV_NUM) {
        return OPRT_INVALID_PARM;
    }
    
    return read(uart_devs[port_id].fd, buff, len);
    // --- END: user implements ---
}


/**
 * @brief enable uart rx interrupt and regist interrupt callback
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
VOID tkl_uart_rx_irq_cb_reg(UINT_T port_id, TUYA_UART_IRQ_CB rx_cb)
{
    // --- BEGIN: user implements ---
    if (port_id >= UART_DEV_NUM) {
        return;
    }

    uart_devs[port_id].isr_cb = rx_cb;
    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    // pthread_create(&uart_devs[port_id].id, &attr, uart_dev_irq_handler, &uart_devs[port_id]);
    // pthread_attr_destroy(&attr);
    pthread_create(&uart_devs[port_id].id, NULL, uart_dev_irq_handler, &uart_devs[port_id]);
    // --- END: user implements ---
}


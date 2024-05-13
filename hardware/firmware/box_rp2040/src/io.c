#include "io.h"
#include "io_commands.h"

#include "usb.h"

#include "pico/stdlib.h"

#include <stdint.h>

#define UART_ENABLED (UART_INSTANCE != NULL)

void io_init(void) {
    if (UART_ENABLED) {
        uart_init(UART_INSTANCE, UART_BAUD);
        gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
        gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);
    }
}

void io_say_n(char* buf, int n) {
    if (USB_ENABLED) {
        usb_cdc_write(buf, n);
    }
    if (UART_ENABLED) {
        uart_write_blocking(UART_INSTANCE, buf, n);
    }
}

void io_say(char* buf) {
    io_say_n(buf, strlen(buf));
}

bool is_terminator(char c) {
    return c == '\n' || c == '\r' || c == '\0';
}

void io_handle_char(char chr) {
    static char in_buf[100];
    static int n = 0;

    if (n > sizeof(in_buf) - 2) {
        if (is_terminator(chr)) {
            n = 0;
            in_buf[0] = '\0';
            io_say("line too long\n");
        }
        return;
    }
    in_buf[n++] = chr;
    if (!is_terminator(chr)) {
        return;
    }
    in_buf[n] = '\0';
    n = 0;
    if (in_buf[0] == '\0') {
        return; // empty line
    }

    io_handle_cmd(in_buf);
}

void io_usb_cdc_task(void) {
    int32_t chr;
    while ( (chr = usb_cdc_read_char()) >= 0 ) {
        io_handle_char((char)chr);
    }
}

void io_uart_task(void) {
    if (!UART_ENABLED) {
        return;
    }
    while (uart_is_readable(UART_INSTANCE)) {
        io_handle_char(uart_getc(UART_INSTANCE));
    }
}

void io_task(void) {
    io_usb_cdc_task();
    io_uart_task();
}

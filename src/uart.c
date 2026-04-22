#include "hardware/uart.h" 
#include "hardware/gpio.h" 
#include <stdio.h>  
#include <string.h>
#include "pico/stdlib.h"
#include "sdcard.h"

extern void test_leaderboard(int argc, char *argv[]);

#define BUFSIZE 100
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

void uart_rx_handler();

/*******************************************************************/

void init_uart() {
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
}

void init_uart_irq() {
    uart_set_fifo_enabled(uart0, false);
    uart_set_irqs_enabled(uart0, true, false);
    irq_set_exclusive_handler(UART0_IRQ, uart_rx_handler);
    irq_set_enabled(UART0_IRQ, true);
}

void uart_rx_handler() {
    uart_get_hw(uart0)->icr = UART_UARTICR_RXIC_BITS;

    if(seridx >= BUFSIZE)
    {
        return;
    }

    char c = uart_get_hw(uart0)->dr;
    if(c == 0x0A)
    {
        newline_seen = 1;
    }

    if(c == 8 && seridx >= 0)
    {
        uart_putc_raw(uart0, '\b');
        uart_putc_raw(uart0, ' ');
        uart_putc_raw(uart0, '\b');
        if(seridx > 0)
        {
            seridx--;
        }
        
        serbuf[seridx] = '\0';
    }
    else
    {
        uart_putc(uart0, c);
        serbuf[seridx] = c;
        seridx++;
    }
}

int _read(__unused int handle, char *buffer, int length) {
    while(newline_seen == 0)
    {
        sleep_ms(5);
    }

    newline_seen = 0;
    int ctr = seridx;
    
    for(int i = 0; i < ctr; i++)
    {
        buffer[i] = serbuf[i];
    }    

    seridx = 0;

    return length;
}

int _write(__unused int handle, char *buffer, int length) {
    for(int i = 0; i < length; i++)
    {
        uart_putc_raw(uart0, buffer[i]);
    }
    
    return length;
}

/*******************************************************************/

struct commands_t {
    const char *cmd;
    void      (*fn)(int argc, char *argv[]);
};

struct commands_t cmds[] = {
        { "append", append },
        { "cat", cat },
        { "cd", cd },
        { "date", date },
        { "input", input },
        { "ls", ls },
        { "mkdir", mkdir },
        { "mount", mount },
        { "pwd", pwd },
        { "rm", rm },
        { "restart", restart },
        { "testlb", test_leaderboard }
};

// This function inserts a string into the input buffer and echoes it to the UART
// but whatever is "typed" by this function can be edited by the user.
void insert_echo_string(const char* str) {
    // Print the string and copy it into serbuf, allowing editing
    seridx = 0;
    newline_seen = 0;
    memset(serbuf, 0, BUFSIZE);

    // Print and fill serbuf with the initial string
    for (int i = 0; str[i] != '\0' && seridx < BUFSIZE - 1; i++) {
        char c = str[i];
        uart_write_blocking(uart0, (uint8_t*)&c, 1);
        serbuf[seridx++] = c;
    }
}

void parse_command(const char* input) {
    char *token = strtok(input, " ");
    int argc = 0;
    char *argv[10];
    while (token != NULL && argc < 10) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    int i = 0;
    for(; i<sizeof cmds/sizeof cmds[0]; i++) {
        if (strcmp(cmds[i].cmd, argv[0]) == 0) {
            cmds[i].fn(argc, argv);
            break;
        }
    }
    if (i == (sizeof cmds/sizeof cmds[0])) {
        printf("Unknown command: %s\n", argv[0]);
    }
}

void command_shell() {
    char input[100];
    memset(input, 0, sizeof(input));

    // Disable buffering for stdout
    setbuf(stdout, NULL);

    printf("\nEnter current ");
    insert_echo_string("date 20250701120000");
    fgets(input, 99, stdin);
    input[strcspn(input, "\r")] = 0; // Remove CR character
    input[strcspn(input, "\n")] = 0; // Remove newline character
    parse_command(input);
    
    printf("SD Card Command Shell");
    printf("\r\nType 'mount' to mount the SD card.\n");
    while (1) {
        printf("\r\n> ");
        fgets(input, sizeof(input), stdin);
        fflush(stdin);
        input[strcspn(input, "\r")] = 0; // Remove CR character
        input[strcspn(input, "\n")] = 0; // Remove newline character
        
        parse_command(input);
    }
}
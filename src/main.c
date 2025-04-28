#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "uart.h"
#include "gpio.h"
#include "clock.h"
#include "program.h"

// --- Global Variables ---
volatile uint8_t run_script_on_pb3 = 0;
volatile uint8_t is_executing_interrupt = 0;
volatile uint8_t startup_finished = 0;
int interrupt_start_ip = -1;
uint8_t global_counter = 1;
uint32_t last_interrupt_time = 0;

// --- External debounce variables ---
extern volatile uint8_t button_last_state;
extern volatile uint8_t button_stable_state;
extern volatile uint32_t debounce_counter;

// --- Constants ---
#define MAX_NESTED_LOOPS 4

typedef struct {
    int start_ip;
    uint8_t count;
} LoopContext;

// --- Function Declarations ---
static int run_interrupt_block(int start_ip);
static void execute_instruction_block(int *ip_ptr, int until_opcode);

// --- Main Function ---
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    printf("[BOOT] STM32 initialized.\n");
    uint8_t test_msg[] = "UART Test\n";
    HAL_UART_Transmit(&huart2, test_msg, sizeof(test_msg) - 1, HAL_MAX_DELAY);

    int ip = 0;
    while (ip < sizeof(program)) {
        uint8_t opcode = program[ip++];
        if (opcode == 0x07) { // end
            printf("[BOOT] Found end of startup at program[%d]\n", ip - 1);
            break;
        } else if (opcode == 0x06) {
            printf("[BOOT] Unexpected interrupt block inside startup!\n");
            break;
        } else {
            int temp_ip = ip - 1;
            execute_instruction_block(&temp_ip, 0x07); // Only stop at startup end
            ip = temp_ip;
        }
    }
    startup_finished = 1;
    printf("[BOOT] Startup script finished.\n");

    // After startup section, scan for interrupt block
    while (ip < sizeof(program)) {
        uint8_t opcode = program[ip++];
        if (opcode == 0x06) { // on PC13 falling then
            interrupt_start_ip = ip;
            printf("[BOOT] Found interrupt block at program[%d]\n", interrupt_start_ip);
            break;
        }
    }

    while (1) {
        if (run_script_on_pb3 && interrupt_start_ip >= 0 && !is_executing_interrupt) {
            run_script_on_pb3 = 0;
            is_executing_interrupt = 1;
            printf("[ISR] Button pressed. Executing interrupt block at program[%d]\n", interrupt_start_ip);
            run_interrupt_block(interrupt_start_ip);
            is_executing_interrupt = 0;
        }

        HAL_Delay(10); // Sleep lightly
    }
}

// --- Run Interrupt Block ---
static int run_interrupt_block(int start_ip) {
    int ip = start_ip;

    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    printf("[ISR] Running interrupt block at program[%d]\n", start_ip);

    execute_instruction_block(&ip, 0x07);

    printf("[ISR] Interrupt block finished.\n");

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    // Wait until button released
    while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
        HAL_Delay(5);
    }

    button_stable_state = GPIO_PIN_SET;
    button_last_state = GPIO_PIN_SET;
    debounce_counter = 0;

    return ip;
}

// --- Execute a Block of Instructions ---
static void execute_instruction_block(int *ip_ptr, int until_opcode) {
    int ip = *ip_ptr;
    LoopContext loop_stack[MAX_NESTED_LOOPS] = {0};
    int loop_stack_ptr = 0;
    uint8_t executing_if_block = 1;

    while (ip < sizeof(program)) {
        uint8_t opcode = program[ip++];
        printf("[DEBUG] Executing opcode 0x%02X at program[%d]\n", opcode, ip - 1);

        if (opcode == until_opcode) {
            printf("[DEBUG] Reached until_opcode 0x%02X at program[%d]\n", until_opcode, ip - 1);
            break;
        }

        switch (opcode) {
            case 0x01: { // set
                uint8_t pin = program[ip++];
                uint8_t val = program[ip++];
                printf("[DEBUG] Set PA%d = %d\n", pin, val);
                HAL_GPIO_WritePin(GPIOA, 1 << pin, val ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            }
            case 0x02: { // wait
                uint16_t delay = (program[ip] << 8) | program[ip + 1];
                ip += 2;
                printf("[DEBUG] Wait %d ms\n", delay);
                HAL_Delay(delay);
                break;
            }
            case 0x03: { // toggle
                uint8_t pin = program[ip++];
                printf("[DEBUG] Toggle PA%d\n", pin);
                HAL_GPIO_TogglePin(GPIOA, 1 << pin);
                break;
            }
            case 0x04: { // if
                uint8_t checkPin = program[ip++];
                uint8_t expected = program[ip++];
                GPIO_TypeDef *port = (checkPin == 13) ? GPIOC : GPIOB;
                GPIO_PinState state = HAL_GPIO_ReadPin(port, 1 << (checkPin % 16));

                printf("[DEBUG] IF P%c%d == %d (actual: %d)\n",
                    (checkPin == 13) ? 'C' : 'B', checkPin, expected, (state == GPIO_PIN_SET));

                if ((state == GPIO_PIN_SET) == expected) {
                    executing_if_block = 1;
                } else {
                    executing_if_block = 0;
                    while (ip < sizeof(program)) {
                        if (program[ip] == 0x0C || program[ip] == 0x07)
                            break;
                        ip++;
                    }
                }
                break;
            }
            case 0x0C: { // else
                if (executing_if_block) {
                    printf("[DEBUG] Skipping ELSE block.\n");
                    while (ip < sizeof(program)) {
                        if (program[ip] == 0x07)
                            break;
                        ip++;
                    }
                } else {
                    printf("[DEBUG] Executing ELSE block.\n");
                    executing_if_block = 1;
                }
                break;
            }
            case 0x05: { // print
                uint8_t len = program[ip++];
                printf("[DEBUG] Print %d bytes\n", len);
                uart_send_data(&program[ip], len);
                ip += len;
                break;
            }
            case 0x06: { // interrupt marker
                printf("[DEBUG] Found interrupt start marker.\n");
                break;
            }
            case 0x07: { // end
                printf("[DEBUG] End of block\n");
                *ip_ptr = ip;
                return;
            }
            case 0x08: { // loop
                uint8_t count = program[ip++];
                printf("[DEBUG] Loop %d times\n", count);
                if (loop_stack_ptr < MAX_NESTED_LOOPS) {
                    loop_stack[loop_stack_ptr++] = (LoopContext){ip, count};
                } else {
                    printf("[ERROR] Loop stack overflow!\n");
                }
                break;
            }
            case 0x09: { // endloop
                if (loop_stack_ptr > 0) {
                    LoopContext *ctx = &loop_stack[loop_stack_ptr - 1];
                    if (--ctx->count > 0) {
                        printf("[DEBUG] Loop again (%d left)\n", ctx->count);
                        ip = ctx->start_ip;
                    } else {
                        printf("[DEBUG] Loop finished\n");
                        loop_stack_ptr--;
                    }
                } else {
                    printf("[ERROR] endloop without matching loop!\n");
                }
                break;
            }
            case 0x0A: { // printNum
                uint8_t len = program[ip++];
                printf("[DEBUG] PrintNum (len %d)\n", len);
                uart_send_data(&program[ip], len);
                ip += len;
                break;
            }
            case 0x0B: { // printCount
                char buffer[16];
                int len = snprintf(buffer, sizeof(buffer), "%d", global_counter++);
                printf("[DEBUG] PrintCount %d\n", global_counter - 1);
                uart_send_data((uint8_t *)buffer, len);
                break;
            }
            default:
                printf("[ERROR] Unknown opcode 0x%02X at program[%d]\n", opcode, ip - 1);
                break;
        }
    }

    *ip_ptr = ip;
}

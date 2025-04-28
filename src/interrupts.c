#include "stm32f4xx_hal.h"
#include "uart.h"
#include <stdio.h>

// --- Variables declared in main.c ---
extern volatile uint8_t run_script_on_pb3;
extern volatile uint8_t is_executing_interrupt;
extern uint32_t last_interrupt_time;

// --- Software debounce variables ---
volatile uint8_t button_last_state = 1; // Not pressed initially (PC13 normally high)
volatile uint8_t button_stable_state = 1;
volatile uint32_t debounce_counter = 0;
const uint32_t debounce_threshold = 50;    // 50 ms debounce (50x1ms ticks)
const uint32_t cooldown_after_press = 300; // 300 ms cooldown after real press

// --- SysTick interrupt handler ---
void SysTick_Handler(void) {
    HAL_IncTick();

    if (is_executing_interrupt) {
        // 🛑 Do not process button while executing interrupt script
        return;
    }

    uint32_t now = HAL_GetTick();
    if (now - last_interrupt_time < cooldown_after_press) {
        // 🛑 Still in cooldown after last valid press
        return;
    }

    uint8_t current_state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

    if (current_state != button_last_state) {
        debounce_counter = 0; // Reset debounce counter if state changed
    } else {
        if (debounce_counter < debounce_threshold) {
            debounce_counter++;
            if (debounce_counter >= debounce_threshold) {
                if (button_stable_state != current_state) {
                    button_stable_state = current_state;

                    if (button_stable_state == GPIO_PIN_RESET) {
                        printf("[DEBUG] Button press detected by software debounce!\n");
                        run_script_on_pb3 = 1;
                        last_interrupt_time = now;
                    }
                }
            }
        }
    }

    button_last_state = current_state;
}

// --- USART2 interrupt handler (for UART communication) ---
void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart2);
}

// --- EXTI line 15..10 interrupt handler (for B1 button = PC13) ---
void EXTI15_10_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);

        // Set the script execution flag
        run_script_on_pb3 = 1;
    }
}

// --- HAL GPIO EXTI Callback (optional if you want future extensions) ---
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    (void)GPIO_Pin; // Not used currently, but kept clean
}

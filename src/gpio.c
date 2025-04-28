#include "gpio.h"

void MX_GPIO_Init(void) {
    // --- Enable clocks ---
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // --- PA5 (LED) ---
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;    // Output push-pull for LED
    GPIO_InitStruct.Pull = GPIO_NOPULL;             // No pull-up/down needed for output
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;    // Low speed
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // --- PC13 (User Button B1) ---
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;         // 🛠️ Just regular input
    GPIO_InitStruct.Pull = GPIO_PULLUP;              // ✅ Internal pull-up enabled
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // 🛑 No EXTI setup needed, because software debounce will handle the button.
}

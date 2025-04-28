#include "uart.h"

UART_HandleTypeDef huart2;

void MX_USART2_UART_Init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&huart2);
}

void uart_send_string(const char *str) {
    while (*str) {
        while (!(huart2.Instance->SR & USART_SR_TXE));
        huart2.Instance->DR = (*str++);
    }
    while (!(huart2.Instance->SR & USART_SR_TC));
}

void uart_send_data(const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        while (!(huart2.Instance->SR & USART_SR_TXE));
        huart2.Instance->DR = data[i];
    }
    while (!(huart2.Instance->SR & USART_SR_TC));
}

int _write(int file, char *data, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)data, len, HAL_MAX_DELAY);
    return len;
}

// uart.h
#ifndef UART_H
#define UART_H

#include "stm32f4xx_hal.h"
#include <stddef.h>  // For size_t

extern UART_HandleTypeDef huart2;

void MX_USART2_UART_Init(void);
void uart_send_string(const char *str);
void uart_send_data(const uint8_t *data, size_t length);

#endif

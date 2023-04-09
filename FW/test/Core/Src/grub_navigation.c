#include "stm32f0xx_hal.h"

void moveUp(UART_HandleTypeDef huart) {
	// 'escape' + '[' + 'A'
	HAL_UART_Transmit(&huart, 0x1b, 1, 10);
	HAL_UART_Transmit(&huart, 0x5b, 1, 10);
	HAL_UART_Transmit(&huart, 0x41, 1, 10);
}

void moveDown(UART_HandleTypeDef huart) {
	// 'escape' + '[' + 'B'
	HAL_UART_Transmit(&huart, 0x1b, 1, 10);
	HAL_UART_Transmit(&huart, 0x5b, 1, 10);
	HAL_UART_Transmit(&huart, 0x42, 1, 10);
}

void moveRight(UART_HandleTypeDef huart) {
	// 'escape' + '[' + 'C'
	HAL_UART_Transmit(&huart, 0x1b, 1, 10);
	HAL_UART_Transmit(&huart, 0x5b, 1, 10);
	HAL_UART_Transmit(&huart, 0x43, 1, 10);
}

void moveLeft(UART_HandleTypeDef huart) {
	// 'escape' + '[' + 'D'
	HAL_UART_Transmit(&huart, 0x1b, 1, 10);
	HAL_UART_Transmit(&huart, 0x5b, 1, 10);
	HAL_UART_Transmit(&huart, 0x44, 1, 10);
}

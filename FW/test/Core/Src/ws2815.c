#include "stm32f0xx_hal.h"

#define T0H 250 // 0-code, High-level time, 220ns to 380ns
#define T1H 600 // 1-code, High-level time, 580ns to 1.6us
#define T0L 600 // 0-code, Low-level time, 580ns to 1.6us
#define T1L 250 // 1-code, Low-level time, 220ns to 420ns
#define TRESET 300 // > 280us

/*void delay_us(uint16_t us, TIM_HandleTypeDef tim) {
 __HAL_TIM_SET_COUNTER(&tim, 0);
 while (__HAL_TIM_GET_COUNTER(&tim) < us)
 ;
 }*/

void encodeColorChannel(uint8_t r, uint8_t g, uint8_t b) { // sends the given uint8 color channel MSB order
	// Data transmit order is GRB, most significant bit first

	encodeBit((g && (1 << 7)) >> 7);
	encodeBit((g && (1 << 6)) >> 6);
	encodeBit((g && (1 << 5)) >> 5);
	encodeBit((g && (1 << 4)) >> 4);
	encodeBit((g && (1 << 3)) >> 3);
	encodeBit((g && (1 << 2)) >> 2);
	encodeBit((g && (1 << 1)) >> 1);

	encodeBit((r && (1 << 7)) >> 7);
	encodeBit((r && (1 << 6)) >> 6);
	encodeBit((r && (1 << 5)) >> 5);
	encodeBit((r && (1 << 4)) >> 4);
	encodeBit((r && (1 << 3)) >> 3);
	encodeBit((r && (1 << 2)) >> 2);
	encodeBit((r && (1 << 1)) >> 1);

	encodeBit((b && (1 << 7)) >> 7);
	encodeBit((b && (1 << 6)) >> 6);
	encodeBit((b && (1 << 5)) >> 5);
	encodeBit((b && (1 << 4)) >> 4);
	encodeBit((b && (1 << 3)) >> 3);
	encodeBit((b && (1 << 2)) >> 2);
	encodeBit((b && (1 << 1)) >> 1);
}

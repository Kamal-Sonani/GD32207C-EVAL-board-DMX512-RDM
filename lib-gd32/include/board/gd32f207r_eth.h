/**
 * @file gd32f207r_eth.h
 *
 */
/* Copyright (C) 2021 by Arjan van Vught mailto:info@orangepi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef BOARD_GD32F207R_ETH_H_
#define BOARD_GD32F207R_ETH_H_

#if !defined(BOARD_GD32F207R_ETH)
# error This file should not be included
#endif

#include "mcu/gd32f20x.h"

typedef enum {
	LED1 = 0, LED2 = 1, LED3 = 2
} led_typedef_enum;

#define LEDn				3U

#define LED1_PIN			GPIO_PIN_0
#define LED1_GPIO_PORT 		GPIOC
#define LED1_GPIO_CLK		RCU_GPIOC

#define LED2_PIN			GPIO_PIN_2
#define LED2_GPIO_PORT		GPIOC
#define LED2_GPIO_CLK		RCU_GPIOC

#define LED3_PIN 			GPIO_PIN_3
#define LED3_GPIO_PORT		GPIOC
#define LED3_GPIO_CLK		RCU_GPIOC

/**
 * I2C
 */

#define I2C_PERIPH			I2C0_PERIPH
#define I2C_RCU_CLK			I2C0_RCU_CLK
#define I2C_GPIO_SCL_PORT	I2C0_GPIO_SCL_PORT
#define I2C_GPIO_SCL_CLK	I2C0_GPIO_SCL_CLK
#define I2C_GPIO_SDA_PORT	I2C0_GPIO_SDA_PORT
#define I2C_GPIO_SDA_CLK	I2C0_GPIO_SDA_CLK
#define I2C_SCL_PIN			I2C0_SCL_PIN
#define I2C_SDA_PIN			I2C0_SDA_PIN

#include "gd32_gpio.h"

#define G32_BOARD_NAME			"GD32F207R_ETH"
#define G32_BOARD_STATUS_LED	GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 0)	///< PC0

#define G32_BOARD_LED1		GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 0)	///< PC0
#define G32_BOARD_LED2		GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 2)	///< PC2
#define G32_BOARD_LED3		GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 3)	///< PC3

typedef enum GD32_BOARD_GD32F207R_ETH {
	// 1 3V3
	GPIO_EXT_3  = GD32_PORT_TO_GPIO(G32_GPIO_PORTB, 7),		///< I2C0 SCA, PB7
	GPIO_EXT_5  = GD32_PORT_TO_GPIO(G32_GPIO_PORTB, 6),		///< I2C0 SDL, PB6
	GPIO_EXT_7  = GD32_PORT_TO_GPIO(G32_GPIO_PORTA, 6),		///< PA6
	GPIO_EXT_11 = GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 7),		///< USART5 RX, PC7
	GPIO_EXT_13 = GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 6),		///< USART5 TX, PC6
	GPIO_EXT_15 = GD32_PORT_TO_GPIO(G32_GPIO_PORTB, 14),	///< PB14
	// 17 3V3
	GPIO_EXT_19 = GD32_PORT_TO_GPIO(G32_GPIO_PORTB, 5),		///< SPI2 MOSI, PB5
	GPIO_EXT_21 = GD32_PORT_TO_GPIO(G32_GPIO_PORTB, 4),		///< SPI2 MISO, PB4
	GPIO_EXT_23 = GD32_PORT_TO_GPIO(G32_GPIO_PORTB, 3),		///< SPI2 SCLK, PB3
	// 2, 4 5V
	// 6 GND
	GPIO_EXT_8  = GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 10),	///< USART2 TX, PC10
	GPIO_EXT_10 = GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 11),	///< USART2 RX, PC11
	GPIO_EXT_12 = GD32_PORT_TO_GPIO(G32_GPIO_PORTB, 10),	///< PB10
	// 14 GND
	GPIO_EXT_16 = GD32_PORT_TO_GPIO(G32_GPIO_PORTB, 15),	///< PB15
	GPIO_EXT_18 = GD32_PORT_TO_GPIO(G32_GPIO_PORTA, 13),	///< PA13
	GPIO_EXT_22 = GD32_PORT_TO_GPIO(G32_GPIO_PORTA, 11),	///< PA11
	GPIO_EXT_24 = GD32_PORT_TO_GPIO(G32_GPIO_PORTA, 15),	///< SPI2 NSS, PA15
	GPIO_EXT_26 = GD32_PORT_TO_GPIO(G32_GPIO_PORTA, 14)		///< PA14
} _gpio_pin;

#endif /* BOARD_GD32F207R_ETH_H_ */

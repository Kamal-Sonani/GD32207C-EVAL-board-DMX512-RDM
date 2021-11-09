/**
 * @file gd32f207c_eval.h
 *
 */
/* Copyright (C) 2021 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32F207C_EVAL_H
#define GD32F207C_EVAL_H

#if !defined(BOARD_GD32F207C_EVAL)
# error This file should not be included
#endif

#if defined (MCU_GD32F20X_H_)
# error This file should be included later
#endif

/**
 * LEDs
 */

#define LED2_PIN            GPIO_PIN_0
#define LED2_GPIO_PORT      GPIOC
#define LED2_GPIO_CLK       RCU_GPIOC

#define LED3_PIN            GPIO_PIN_2
#define LED3_GPIO_PORT      GPIOC
#define LED3_GPIO_CLK       RCU_GPIOC

#define LED4_PIN            GPIO_PIN_0
#define LED4_GPIO_PORT      GPIOE
#define LED4_GPIO_CLK       RCU_GPIOE

#define LED5_PIN            GPIO_PIN_1
#define LED5_GPIO_PORT      GPIOE
#define LED5_GPIO_CLK       RCU_GPIOE

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

/**
 * SPI
 */

#define SPI2_REMAP
#if defined (SPI2_REMAP)
# define SPI_REMAP
#endif
#define SPI_PERIPH			SPI2_PERIPH
#define SPI_NSS_GPIO_PORT	SPI2_NSS_GPIO_PORT
#define SPI_NSS_GPIO_CLK	SPI2_NSS_GPIO_CLK
#define SPI_NSS_PIN			SPI2_NSS_PIN
#define SPI_RCU_CLK			SPI2_RCU_CLK
#define SPI_GPIO_PORT		SPI2_GPIO_PORT
#define SPI_GPIO_CLK		SPI2_GPIO_CLK
#define SPI_SCK_PIN			SPI2_SCK_PIN
#define SPI_MISO_PIN		SPI2_MISO_PIN
#define SPI_MOSI_PIN		SPI2_MOSI_PIN

/**
 * U(S)ART
 */

// #define USART0_REMAP
#define USART1_REMAP
#define USART2_FULL_REMAP
// #define USART2_PARTIAL_REMAP
// #define UART3_REMAP
// #define USART5_REMAP
// #define UART6_REMAP

#include "mcu/gd32f20x.h"
#include "gd32_gpio.h"

#define GD32_BOARD_NAME		"GD32F207C_EVAL"

#define GD32_BOARD_LED1		GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 0)	///< PC0
#define GD32_BOARD_LED2		GD32_PORT_TO_GPIO(G32_GPIO_PORTC, 2)	///< PC2
#define GD32_BOARD_LED3		GD32_PORT_TO_GPIO(G32_GPIO_PORTE, 0)	///< PE0
#define GD32_BOARD_LED4		GD32_PORT_TO_GPIO(G32_GPIO_PORTE, 1)	///< PE1

/**
 * Below is for (backwards) compatibility with Orange Pi Zero board.
 */

typedef enum GD32_BOARD_GD32207C_EVAL {
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

#endif /* GD32F207C_EVAL_H */

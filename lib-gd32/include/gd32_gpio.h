/**
 * @file gd32_gpio.h
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

#ifndef GD32_GPIO_H_
#define GD32_GPIO_H_

#define GD32_PORT_TO_GPIO(p,n)	((p * 16) + n)
#define GD32_GPIO_TO_PORT(g)	(g / 16)
#define GD32_GPIO_TO_NUMBER(g)	(g - (16 * GD32_GPIO_TO_PORT(g)))

typedef enum T_GD32_Pn {
	G32_GPIO_PORTA = 0,
	G32_GPIO_PORTB,
	G32_GPIO_PORTC,
	G32_GPIO_PORTD,
	G32_GPIO_PORTE,
	G32_GPIO_PORTF,
	G32_GPIO_PORTG,
} G32_Pn_TypeDef;

#include <stdint.h>

#include "gd32f20x.h"
#include "gd32f20x_gpio.h"

#define GPIO_FSEL_OUTPUT	GPIO_MODE_OUT_PP

#ifdef __cplusplus
extern "C" {
#endif

inline static void gd32_gpio_fsel(uint32_t gpio, uint32_t fsel) {
	const uint32_t gpio_periph = GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
	const uint32_t pin = BIT(GD32_GPIO_TO_NUMBER(gpio));
	gpio_init(gpio_periph, fsel, GPIO_OSPEED_50MHZ, pin);
}

inline static void gd32_gpio_clr(uint32_t gpio) {
	const uint32_t gpio_periph = GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
	const uint32_t pin =  BIT(GD32_GPIO_TO_NUMBER(gpio));
	GPIO_BC(gpio_periph) = (uint32_t) pin;
}

inline static void gd32_gpio_set(uint32_t gpio) {
	const uint32_t gpio_periph = GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
	const uint32_t pin =  BIT(GD32_GPIO_TO_NUMBER(gpio));
	GPIO_BOP(gpio_periph) = (uint32_t) pin;
}

#ifdef __cplusplus
}
#endif

#endif /* GD32_GPIO_H_ */

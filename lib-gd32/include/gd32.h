/**
 * @file gd32.h
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

#ifndef GD32_H_
#define GD32_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void udelay(uint16_t);

#if defined (GD32F20X_CL)
# include "gd32f20x_adc.h"
# include "gd32f20x_bkp.h"
# include "gd32f20x_dma.h"
# include "gd32f20x_fmc.h"
# include "gd32f20x_fwdgt.h"
# include "gd32f20x_gpio.h"
# include "gd32f20x_misc.h"
# include "gd32f20x_pmu.h"
# include "gd32f20x_rcu.h"
# include "gd32f20x_rtc.h"
# include "gd32f20x_timer.h"
# include "gd32f20x_usart.h"
#else
# error
#endif

#ifdef __cplusplus
}
#endif

#include "gd32_board.h"

#endif /* GD32_H_ */

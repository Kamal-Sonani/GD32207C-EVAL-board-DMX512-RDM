/**
 * @file pixeldmxstartstop.h
 *
 */
/* Copyright (C) 2020-2021 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#ifndef H3_WS28XXDMXSTARTSTOP_H_
#define H3_WS28XXDMXSTARTSTOP_H_

#include "pixeldmxhandler.h"

#include "h3_gpio.h"
#include "board/h3_opi_zero.h"

#define GPIO_START_STOP		GPIO_EXT_12

class PixelDmxStartStop final: public PixelDmxHandler {
public:
	PixelDmxStartStop() {
		h3_gpio_fsel(GPIO_START_STOP, GPIO_FSEL_OUTPUT);
		h3_gpio_clr(GPIO_START_STOP);
	}

	~PixelDmxStartStop() override {}

	void Start() override {
		h3_gpio_set(GPIO_START_STOP);
	}

	void Stop() override {
		h3_gpio_clr(GPIO_START_STOP);
	}
};

#endif /* H3_WS28XXDMXSTARTSTOP_H_ */

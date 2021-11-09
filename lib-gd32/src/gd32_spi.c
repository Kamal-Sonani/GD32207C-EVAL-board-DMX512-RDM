/**
 * @file gd32_spi.c
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

//TODO gd32_spi.c

#include <stdint.h>
#include <stdbool.h>

#include "gd32_spi.h"

#if 0

__attribute__((weak)) void gd32_spi_begin(void)  {

}

__attribute__((weak)) void gd32_spi_set_speed_hz(uint32_t speed_hz) {

}

/**
 * DMA
 */

__attribute__((weak)) const uint8_t *gd32_spi_dma_tx_prepare(uint32_t *data_length) {
	return 0;
}

__attribute__((weak)) void gd32_spi_dma_tx_start(const uint8_t *tx_buffer, uint32_t length) {

}

__attribute__((weak)) bool gd32_spi_dma_tx_is_active(void) {
	return false;
}
#endif

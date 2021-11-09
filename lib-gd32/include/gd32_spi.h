/**
 * @file gd32_spi.h
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

#ifndef GD32_SPI_H_
#define GD32_SPI_H_

typedef enum GD32_SPI_CS {
	GD32_SPI_CS0 = 0,		///< Chip Select 0
	GD32_SPI_CS1 = 1,		///< Chip Select 1
	GD32_SPI_CS2 = 2,		///< Chip Select 2
	GD32_SPI_CS3 = 3,		///< Chip Select 3
	GD32_SPI_CS_NONE = 4	///< No CS, control it yourself
} gd32_spi_chip_select_t;

#ifdef __cplusplus
extern "C" {
#endif

extern void gd32_spi_begin(void);
//extern void gd32_spi_end(void);

extern void gd32_spi_set_speed_hz(uint32_t speed_hz);
extern void gd32_spi_setDataMode(uint8_t mode);
extern void gd32_spi_chipSelect(uint8_t chip_select);
//extern void gd32_spi_setChipSelectPolarity(uint8_t chip_select, uint8_t polarity);

//extern uint8_t gd32_spi_transfer(uint8_t data);
//extern void gd32_spi_transfernb(char *tx_buffer, char *rx_buffer, uint32_t data_length);
extern void gd32_spi_transfern(char *tx_buffer, uint32_t data_length);

extern void gd32_spi_write(uint16_t data);
extern void gd32_spi_writenb(const char *tx_buffer, uint32_t data_length);

/*
 * DMA support
 */

extern void gd32_spi_dma_begin(void);
extern void gd32_spi_dma_set_speed_hz(uint32_t speed_hz);
extern const uint8_t *gd32_spi_dma_tx_prepare(uint32_t *data_length);
extern void gd32_spi_dma_tx_start(const uint8_t *tx_buffer, uint32_t length);
extern bool gd32_spi_dma_tx_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_GD32_SPI_H_ */

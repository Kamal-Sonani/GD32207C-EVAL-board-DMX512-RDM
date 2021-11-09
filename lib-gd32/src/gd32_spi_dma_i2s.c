/**
 * @file gd32_spi_dma_i2s.c
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

#include <stdint.h>
#include <assert.h>

#include "gd32.h"

static uint16_t s_TxBuffer[1024] __attribute__ ((aligned (4)));

static void spi_i2s_dma_config(void) {
	dma_parameter_struct dma_init_struct;
	/* enable DMA1 */
	rcu_periph_clock_enable(RCU_DMA1);

	dma_deinit(DMA1, DMA_CH1);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
	dma_init_struct.periph_addr = SPI2 + 0x0CU;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(DMA1, DMA_CH1, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(DMA1, DMA_CH1);
	dma_memory_to_memory_disable(DMA1, DMA_CH1);

	dma_interrupt_flag_clear(DMA1, DMA_CH1, DMA_INT_FTF);
	dma_interrupt_enable(DMA1, DMA_CH1, DMA_INT_FTF);

//	NVIC_SetPriority(DMA1_Channel1_IRQn, 0);
//	NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

void gd32_spi_dma_begin(void) {
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_clock_enable(RCU_SPI2);

	/**
	 * SPI2_REMAP = 1
	 *
	 * SPI2_NSS/ I2S2_WS		PA4
	 * SPI2_SCK/ I2S2_CK 		PC10
	 * SPI2_MISO 				PC11
	 * SPI2_MOSI/I2S2_SD 		PC12
	 */

	gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
	gpio_pin_remap_config(GPIO_SPI2_REMAP, ENABLE);

	/**
	 * Setup PLL2
	 *
	 * clks=14
	 * i2sclock=160000000
	 * i2sdiv=12, i2sof=256
	 */

	rcu_pll2_config(RCU_PLL2_MUL16);
	/* enable PLL2 */
	RCU_CTL |= RCU_CTL_PLL2EN;
	/* wait till PLL2 is ready */
	while ((RCU_CTL & RCU_CTL_PLL2STB) == 0U) {
	}
	rcu_i2s2_clock_config(RCU_I2S2SRC_CKPLL2_MUL2);

	i2s_psc_config(SPI2, 200000, I2S_FRAMEFORMAT_DT16B_CH16B,  I2S_MCKOUT_DISABLE);
	i2s_init(SPI2, I2S_MODE_MASTERTX, I2S_STD_MSB, I2S_CKPL_LOW);
	i2s_enable(SPI2);

	spi_i2s_dma_config();
}

void gd32_spi_dma_set_speed_hz(uint32_t nSpeedHz) {
//	const uint32_t audiosample = nSpeedHz / 16 / 2 ;
//	i2s_psc_config(SPI2, audiosample, I2S_FRAMEFORMAT_DT16B_CH16B,  I2S_MCKOUT_DISABLE);
}

/**
 * DMA
 */

const uint8_t *gd32_spi_dma_tx_prepare(uint32_t *nLength) {
	*nLength = (sizeof(s_TxBuffer) / sizeof(s_TxBuffer[0])) * 2;
	return (const uint8_t *)s_TxBuffer;
}

void gd32_spi_dma_tx_start(const uint8_t *pTxBuffer, uint32_t nLength) {
	assert(((uint32_t)pTxBuffer & 0x1) != 0x1);
	assert((uint32_t)pTxBuffer >= (uint32_t)s_TxBuffer);
	assert(nLength != 0);

	uint8_t *tx = (uint8_t *)pTxBuffer;

	tx[0] = 0;

	const uint32_t dma_chcnt = (((nLength + 1) / 2) & DMA_CHANNEL_CNT_MASK);
	uint16_t *p = (uint16_t *)pTxBuffer;

	for (uint32_t i = 0; i < dma_chcnt; i++) {
		p[i] = __builtin_bswap16(p[i]);
	}

//	dma_channel_disable(DMA1, DMA_CH1);
	uint32_t nDmaChCTL = DMA_CHCTL(DMA1, DMA_CH1);
	nDmaChCTL &= ~DMA_CHXCTL_CHEN;
	DMA_CHCTL(DMA1, DMA_CH1) = nDmaChCTL;
	DMA_CHMADDR(DMA1, DMA_CH1) = (uint32_t)pTxBuffer;
	DMA_CHCNT(DMA1, DMA_CH1) = dma_chcnt;
//	dma_channel_enable(DMA1, DMA_CH1);
	nDmaChCTL |= DMA_CHXCTL_CHEN;
	DMA_CHCTL(DMA1, DMA_CH1) = nDmaChCTL;
	spi_dma_enable(SPI2, SPI_DMA_TRANSMIT);
}

bool gd32_spi_dma_tx_is_active(void) {
	return FALSE;
}

/**
 * /CS
 */

//TODO Implement /CS

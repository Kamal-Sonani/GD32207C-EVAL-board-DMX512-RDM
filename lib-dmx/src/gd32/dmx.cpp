/**
 * @file dmx.cpp
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

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cassert>

#include "dmxmulti.h"
#include "dmxconst.h"
#include "dmx_internal.h"

#include "rdm.h"
#include "rdm_e120.h"

#include "gd32.h"

#include "debug.h"

#ifndef NDEBUG		// Logic Analyzer
# define PE0_LOW()	gpio_bit_reset(GPIOE, GPIO_PIN_0)
# define PE0_HIGH()	gpio_bit_set(GPIOE, GPIO_PIN_0)
# define PE1_LOW()	gpio_bit_reset(GPIOE, GPIO_PIN_1)
# define PE1_HIGH()	gpio_bit_set(GPIOE, GPIO_PIN_1)
#else
# define PE0_LOW()
# define PE0_HIGH()
# define PE1_LOW()
# define PE1_HIGH()
#endif

#ifndef ALIGNED
# define ALIGNED __attribute__ ((aligned (4)))
#endif

namespace dmxmulti {
enum class TxRxState {
	IDLE,
	PRE_BREAK,
	BREAK,
	MAB,
	DMXDATA,
	RDMDATA,
	CHECKSUMH,
	CHECKSUML,
	RDMDISCFE,
	RDMDISCEUID,
	RDMDISCECS,
	DMXINTER
};

enum class PortState {
	IDLE = 0, TX, RX
};

struct TxData {
	uint8_t data[dmx::buffer::SIZE];	// multiple of uint16_t
	uint16_t nLength;
};

struct RxRdmStatistics {
	uint16_t nIndex;
	uint16_t nChecksum;					// This must be uint16_t
	uint16_t nDiscIndex;
};

struct RxDmxPackets {
	uint32_t nPerSecond;
	uint32_t nCount;
	uint32_t nCountPrevious;
	uint32_t nTimerCounterPrevious;
};

struct RxData {
	uint8_t data[dmx::buffer::SIZE];	// multiple of uint16_t
	union {
		RxRdmStatistics Rdm;
		Statistics Dmx;
	};
	volatile TxRxState State;
};

struct DirGpio {
	uint32_t nPort;
	uint32_t nPin;
};
}  // namespace dmxmulti

using namespace dmxmulti;
using namespace dmx;

static constexpr DirGpio s_DirGpio[DMX_MAX_PORTS] = {
		{ config::DIR_PORT_0_GPIO_PORT, config::DIR_PORT_0_GPIO_PIN },
#if DMX_MAX_PORTS >= 2
		{ config::DIR_PORT_1_GPIO_PORT, config::DIR_PORT_1_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 3
		{ config::DIR_PORT_2_GPIO_PORT, config::DIR_PORT_2_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 4
		{ config::DIR_PORT_3_GPIO_PORT, config::DIR_PORT_3_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 5
		{ config::DIR_PORT_4_GPIO_PORT, config::DIR_PORT_4_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 6
		{ config::DIR_PORT_5_GPIO_PORT, config::DIR_PORT_5_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 7
		{ config::DIR_PORT_6_GPIO_PORT, config::DIR_PORT_6_GPIO_PIN },
#endif
#if DMX_MAX_PORTS == 8
		{ config::DIR_PORT_7_GPIO_PORT, config::DIR_PORT_7_GPIO_PIN },
#endif
};

static volatile PortState sv_PortState[dmxmulti::config::max::OUT] ALIGNED;

// DMX RX

static volatile RxDmxPackets sv_nRxDmxPackets[dmxmulti::config::max::IN] ALIGNED;

// DMX RDM RX

static RxData s_RxBuffer[dmxmulti::config::max::IN] ALIGNED;

// DMX TX

static TxData s_TxBuffer[dmxmulti::config::max::OUT];

static uint32_t s_nDmxTransmitBreakTime { dmx::transmit::BREAK_TIME_MIN };
static uint32_t s_nDmxTransmitMabTime { dmx::transmit::MAB_TIME_MIN };		///< MAB_TIME_MAX = 1000000U;
static uint32_t s_nDmxTransmitPeriod { dmx::transmit::PERIOD_DEFAULT };
static uint16_t s_nUartsSending;

void gpio_config() {
#ifndef NDEBUG
	rcu_periph_clock_enable(RCU_GPIOE);
	gpio_init(GPIOE, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1);

	PE0_LOW();
	PE1_LOW();
#endif
}

void irq_handler_dmx_rdm_input(const uint32_t uart, const uint32_t nPortIndex) {
	__DMB();
	uint16_t nIndex;
	uint32_t nCounter;
	PE0_HIGH();

	if(RESET != (USART_REG_VAL(uart, USART_FLAG_FERR) & BIT(USART_BIT_POS(USART_FLAG_FERR)))){
		USART_REG_VAL(uart, USART_FLAG_FERR) &= ~BIT(USART_BIT_POS(USART_FLAG_FERR));
		static_cast<void>(GET_BITS(USART_DATA(uart), 0U, 8U));
		s_RxBuffer[nPortIndex].State = TxRxState::BREAK;
		PE0_LOW();
		return;
	}

	const auto data = static_cast<uint16_t>(GET_BITS(USART_DATA(uart), 0U, 8U));

	switch (s_RxBuffer[nPortIndex].State) {
	case TxRxState::IDLE:
		s_RxBuffer[nPortIndex].State = TxRxState::RDMDISCFE;
		s_RxBuffer[nPortIndex].data[0] = data;
		s_RxBuffer[nPortIndex].Rdm.nIndex = 1;
		break;
	case TxRxState::BREAK:
		switch (data) {
		case START_CODE:
			s_RxBuffer[nPortIndex].data[0] = START_CODE;
			s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket = 1;
			sv_nRxDmxPackets[nPortIndex].nCount++;
			sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious = TIMER_CNT(TIMER2);

			s_RxBuffer[nPortIndex].State = TxRxState::DMXDATA;
			PE1_HIGH();
			break;
		case E120_SC_RDM:
			s_RxBuffer[nPortIndex].data[0] = E120_SC_RDM;
			s_RxBuffer[nPortIndex].Rdm.nChecksum = E120_SC_RDM;
			s_RxBuffer[nPortIndex].Rdm.nIndex = 1;

			s_RxBuffer[nPortIndex].State = TxRxState::RDMDATA;
			break;
		default:
			s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			break;
		}
		break;
	case TxRxState::DMXDATA:
		nIndex = s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket;
		s_RxBuffer[nPortIndex].data[nIndex] = data;
		s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket++;

		if (s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket > dmx::max::CHANNELS) {
			s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket |= 0x8000;
			s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			PE1_LOW();
			break;
		}

#if DMX_MAX_PORTS >= 5
		if (nPortIndex <= 3) {
#endif
			 nCounter = TIMER_CNT(TIMER2);
#if DMX_MAX_PORTS >= 5
		}
		else {
			nCounter = TIMER_CNT(TIMER3);
		}
#endif
		{
			const auto nDelta = nCounter - sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious;
			sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious = nCounter;
			const auto nPulse = nCounter + nDelta + 4;

		    switch(nPortIndex){
		    case 0:
		        TIMER_CH0CV(TIMER2) = nPulse;
		        break;
#if DMX_MAX_PORTS >= 2
		    case 1:
		        TIMER_CH1CV(TIMER2) = nPulse;
		        break;
#endif
#if DMX_MAX_PORTS >= 3
		    case 2:
		        TIMER_CH2CV(TIMER2) = nPulse;
		        break;
#endif
#if DMX_MAX_PORTS >= 4
		    case 3:
		         TIMER_CH3CV(TIMER2) = nPulse;
		        break;
#endif
#if DMX_MAX_PORTS >= 5
		    case 4:
		        TIMER_CH0CV(TIMER3) = nPulse;
		        break;
#endif
#if DMX_MAX_PORTS >= 6
		    case 5:
		        TIMER_CH1CV(TIMER3) = nPulse;
		        break;
#endif
#if DMX_MAX_PORTS >= 7
		    case 6:
		        TIMER_CH2CV(TIMER3) = nPulse;
		        break;
#endif
#if DMX_MAX_PORTS == 8
		    case 7:
		         TIMER_CH3CV(TIMER3) = nPulse;
		        break;
#endif
		    default:
				assert(0);
				__builtin_unreachable();
		        break;
		    }
		}
		break;
	case TxRxState::RDMDATA: {
		nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
		s_RxBuffer[nPortIndex].data[nIndex] = data;
		s_RxBuffer[nPortIndex].Rdm.nIndex++;

		s_RxBuffer[nPortIndex].Rdm.nChecksum = static_cast<uint16_t>(s_RxBuffer[nPortIndex].Rdm.nChecksum + data);

		const auto *p = reinterpret_cast<struct TRdmMessage*>(&s_RxBuffer[nPortIndex].data[0]);

		if (s_RxBuffer[nPortIndex].Rdm.nIndex == p->message_length) {
			s_RxBuffer[nPortIndex].State = TxRxState::CHECKSUMH;
		} else if (s_RxBuffer[nPortIndex].Rdm.nIndex > sizeof(struct TRdmMessage)) {
			s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
		}
	}
		break;
	case TxRxState::CHECKSUMH:
		nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
		s_RxBuffer[nPortIndex].data[nIndex] = data;
		s_RxBuffer[nPortIndex].Rdm.nIndex++;
		s_RxBuffer[nPortIndex].Rdm.nChecksum = static_cast<uint16_t>(s_RxBuffer[nPortIndex].Rdm.nChecksum - static_cast<uint16_t>(data << 8));

		s_RxBuffer[nPortIndex].State = TxRxState::CHECKSUML;
		break;
	case TxRxState::CHECKSUML: {
		nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
		s_RxBuffer[nPortIndex].data[nIndex] = data;
		s_RxBuffer[nPortIndex].Rdm.nIndex++;
		s_RxBuffer[nPortIndex].Rdm.nChecksum = static_cast<uint16_t>(s_RxBuffer[nPortIndex].Rdm.nChecksum - data);

		const auto *p = reinterpret_cast<struct TRdmMessage *>(&s_RxBuffer[nPortIndex].data[0]);

		if (!((s_RxBuffer[nPortIndex].Rdm.nChecksum == 0) && (p->sub_start_code == E120_SC_SUB_MESSAGE))) {
			s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket= 0; // This is correct.
		} else {
			s_RxBuffer[nPortIndex].Rdm.nIndex |= 0x4000;
		}

		s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
	}
		break;
	case TxRxState::RDMDISCFE:
		nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
		s_RxBuffer[nPortIndex].data[nIndex] = data;
		s_RxBuffer[nPortIndex].Rdm.nIndex++;

		if ((data == 0xAA) || (s_RxBuffer[nPortIndex].Rdm.nIndex == 9)) {
			s_RxBuffer[nPortIndex].Rdm.nDiscIndex = 0;
			s_RxBuffer[nPortIndex].State = TxRxState::RDMDISCEUID;
		}
		break;
	case TxRxState::RDMDISCEUID:
		nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
		s_RxBuffer[nPortIndex].data[nIndex] = data;
		s_RxBuffer[nPortIndex].Rdm.nIndex++;
		s_RxBuffer[nPortIndex].Rdm.nDiscIndex++;

		if (s_RxBuffer[nPortIndex].Rdm.nDiscIndex == 2 * RDM_UID_SIZE) {
			s_RxBuffer[nPortIndex].Rdm.nDiscIndex = 0;
			s_RxBuffer[nPortIndex].State = TxRxState::RDMDISCECS;
		}
		break;
	case TxRxState::RDMDISCECS:
		nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
		s_RxBuffer[nPortIndex].data[nIndex] = data;
		s_RxBuffer[nPortIndex].Rdm.nIndex++;

		s_RxBuffer[nPortIndex].Rdm.nDiscIndex++;

		if (s_RxBuffer[nPortIndex].Rdm.nDiscIndex == 4) {
			s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
		}
		break;
	default:
		s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket= 0; // This is correct.
		s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
		break;
	}

	PE0_LOW();
	__DMB();
}

extern "C" {
#if defined (DMX_USE_USART0)
void USART0_IRQHandler(void) {
	irq_handler_dmx_rdm_input(USART0, dmxmulti::config::USART0_PORT);
}
#endif
#if defined (DMX_USE_USART1)
void USART1_IRQHandler(void) {
	irq_handler_dmx_rdm_input(USART1, dmxmulti::config::USART1_PORT);
}
#endif
#if defined (DMX_USE_USART2)
void USART2_IRQHandler(void) {
	irq_handler_dmx_rdm_input(USART2, dmxmulti::config::USART2_PORT);
}
#endif
#if defined (DMX_USE_UART3)
void UART3_IRQHandler(void) {
	irq_handler_dmx_rdm_input(UART3, dmxmulti::config::UART3_PORT);
}
#endif
#if defined (DMX_USE_UART4)
void UART4_IRQHandler(void) {
	irq_handler_dmx_rdm_input(UART4, dmxmulti::config::UART4_PORT);
}
#endif
#if defined (DMX_USE_USART5)
void USART5_IRQHandler(void) {
	irq_handler_dmx_rdm_input(USART5, dmxmulti::config::USART5_PORT);
}
#endif
#if defined (DMX_USE_UART6)
void UART6_IRQHandler(void) {
	irq_handler_dmx_rdm_input(UART6, dmxmulti::config::UART6_PORT);
}
#endif
#if defined (DMX_USE_UART7)
void UART7_IRQHandler(void) {
	irq_handler_dmx_rdm_input(UART7, dmxmulti::config::UART7_PORT);
}
#endif
}

void timer1_config() {
	rcu_periph_clock_enable(RCU_TIMER1);
	timer_deinit(TIMER1);

	timer_parameter_struct timer_initpara;

	timer_initpara.prescaler = 119;	///< TIMER1CLK = SystemCoreClock / 120 = 1MHz => us ticker
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = s_nDmxTransmitPeriod;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER1, &timer_initpara);

	timer_flag_clear(TIMER1, ~0);
	timer_interrupt_flag_clear(TIMER1, ~0);

	timer_channel_output_mode_config(TIMER1, TIMER_CH_0, TIMER_OC_MODE_ACTIVE); // Break
	timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_ACTIVE); // MAB
	timer_channel_output_mode_config(TIMER1, TIMER_CH_2, TIMER_OC_MODE_ACTIVE); // Data

	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, 0);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, s_nDmxTransmitBreakTime);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, s_nDmxTransmitBreakTime + s_nDmxTransmitMabTime);

	timer_interrupt_enable(TIMER1, TIMER_INT_CH0 | TIMER_INT_CH1 | TIMER_INT_CH2);

	NVIC_SetPriority(TIMER1_IRQn, 0);
	NVIC_EnableIRQ(TIMER1_IRQn);

	timer_enable(TIMER1);
}

void timer2_config() {
	rcu_periph_clock_enable(RCU_TIMER2);
	timer_deinit(TIMER2);

	timer_parameter_struct timer_initpara;

	timer_initpara.prescaler = 119;	///< TIMER2CLK = SystemCoreClock / 120 = 1MHz => us ticker
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = static_cast<uint32_t>(~0);
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER2, &timer_initpara);

	timer_flag_clear(TIMER2, ~0);
	timer_interrupt_flag_clear(TIMER2, ~0);

	timer_channel_output_mode_config(TIMER2, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER2, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER2, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER2, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);

	NVIC_SetPriority(TIMER2_IRQn, 0);
	NVIC_EnableIRQ(TIMER2_IRQn);

	timer_enable(TIMER2);
}

void timer3_config() {
#if DMX_MAX_PORTS >= 5
	rcu_periph_clock_enable(RCU_TIMER3);
	timer_deinit(TIMER3);

	timer_parameter_struct timer_initpara;

	timer_initpara.prescaler = 119;	///< TIMER2CLK = SystemCoreClock / 120 = 1MHz => us ticker
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = static_cast<uint32_t>(~0);
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER3, &timer_initpara);

	timer_flag_clear(TIMER3, ~0);
	timer_interrupt_flag_clear(TIMER3, ~0);

	timer_channel_output_mode_config(TIMER3, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER3, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER3, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER3, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);

	NVIC_SetPriority(TIMER3_IRQn, 0);
	NVIC_EnableIRQ(TIMER3_IRQn);

	timer_enable(TIMER3);
#endif
}

void timer6_config() {
	rcu_periph_clock_enable(RCU_TIMER6);
	timer_deinit(TIMER6);

	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler = 11999;	// Source of count clock is internal clock 120 MHz -> 10 KHz
	timer_initpara.period = 10000;		// 1 second
	timer_init(TIMER6, &timer_initpara);

	timer_flag_clear(TIMER6, ~0);
	timer_interrupt_flag_clear(TIMER6, ~0);

	timer_interrupt_enable(TIMER6, TIMER_INT_UP);

	NVIC_SetPriority(TIMER6_IRQn, 0);
	NVIC_EnableIRQ(TIMER6_IRQn);

	timer_enable(TIMER6);
}

void uart_config(uint32_t usart_periph) {
	rcu_periph_clock_enable(RCU_AF);
#ifndef NDEBUG
	auto isSet = false;
#endif
	switch (usart_periph) {
#if defined (DMX_USE_USART0)
	case USART0:
		rcu_periph_clock_enable(USART0_RCU_CLK);
		rcu_periph_clock_enable(USART0_GPIO_CLK);
		gpio_init(USART0_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART0_TX_PIN);
		gpio_init(USART0_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART0_RX_PIN);
# if defined (USART0_REMAP)
		gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);
# endif
# ifndef NDEBUG
		isSet = true;
# endif
		break;
#endif
#if defined (DMX_USE_USART1)
	case USART1:
		rcu_periph_clock_enable(USART1_RCU_CLK);
		rcu_periph_clock_enable(USART1_GPIO_CLK);
		gpio_init(USART1_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN);
		gpio_init(USART1_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART1_RX_PIN);
# if defined (USART1_REMAP)
		gpio_pin_remap_config(GPIO_USART1_REMAP, ENABLE);
# endif
# ifndef NDEBUG
		isSet = true;
# endif
		break;
#endif
#if defined (DMX_USE_USART2)
	case USART2:
		rcu_periph_clock_enable(USART2_RCU_CLK);
		rcu_periph_clock_enable(USART2_GPIO_CLK);
		gpio_init(USART2_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART2_TX_PIN);
		gpio_init(USART2_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART2_RX_PIN);
# if defined (USART2_FULL_REMAP)
		gpio_pin_remap_config(GPIO_USART2_FULL_REMAP, ENABLE);
# endif
# ifndef NDEBUG
		isSet = true;
# endif
		break;
#endif
#if defined (DMX_USE_UART3)
	case UART3:
		rcu_periph_clock_enable(UART3_RCU_CLK);
		rcu_periph_clock_enable(UART3_GPIO_CLK);
		gpio_init(UART3_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART3_TX_PIN);
		gpio_init(UART3_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART3_RX_PIN);
# if defined (UART3_REMAP)
		gpio_pin_remap_config(GPIO_PCF5_UART3_REMAP, ENABLE);
# endif
#ifndef NDEBUG
		isSet = true;
#endif
		break;
#endif
#if defined (DMX_USE_UART4)
	case UART4:
		rcu_periph_clock_enable(UART4_RCU_CLK);
		rcu_periph_clock_enable(UART4_GPIO_TX_CLK);
		rcu_periph_clock_enable(UART4_GPIO_RX_CLK);
		gpio_init(UART4_GPIO_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART4_TX_PIN);
		gpio_init(UART4_GPIO_RX_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART4_RX_PIN);
#ifndef NDEBUG
		isSet = true;
#endif
		break;
#endif
#if defined (DMX_USE_USART5)
	case USART5:
		rcu_periph_clock_enable(USART5_RCU_CLK);
		rcu_periph_clock_enable(USART5_GPIO_CLK);
		gpio_init(USART5_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART5_TX_PIN);
		gpio_init(USART5_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART5_RX_PIN);
# if defined (USART5_REMAP)
		gpio_pin_remap_config(GPIO_PCF5_USART5_TX_PG14_REMAP, ENABLE);
		gpio_pin_remap_config(GPIO_PCF5_USART5_RX_PG9_REMAP, ENABLE);
# endif
#ifndef NDEBUG
		isSet = true;
#endif
		break;
#endif
#if defined (DMX_USE_UART6)
	case UART6:
		rcu_periph_clock_enable(UART6_RCU_CLK);
		rcu_periph_clock_enable(UART6_GPIO_CLK);
		gpio_init(UART6_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART6_TX_PIN);
		gpio_init(UART6_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART6_RX_PIN);
# if defined (UART6_REMAP)
		gpio_pin_remap_config(GPIO_PCF5_UART6_REMAP, ENABLE);
# endif
#ifndef NDEBUG
		isSet = true;
#endif
		break;
#endif
#if defined (DMX_USE_UART7)
	case UART7:
		rcu_periph_clock_enable(UART7_RCU_CLK);
		rcu_periph_clock_enable(UART7_GPIO_CLK);
		gpio_init(UART7_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART7_TX_PIN);
		gpio_init(UART7_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART7_RX_PIN);
#ifndef NDEBUG
		isSet = true;
#endif
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}

	assert(isSet);

	usart_deinit(usart_periph);
	usart_baudrate_set(usart_periph, 250000U);
	usart_word_length_set(usart_periph, USART_WL_8BIT);
	usart_stop_bit_set(usart_periph, USART_STB_2BIT);
	usart_parity_config(usart_periph, USART_PM_NONE);
	usart_hardware_flow_rts_config(usart_periph, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(usart_periph, USART_CTS_DISABLE);
	usart_receive_config(usart_periph, USART_RECEIVE_ENABLE);
	usart_transmit_config(usart_periph, USART_TRANSMIT_ENABLE);
	usart_enable(usart_periph);
}

void usart_dma_config(void) {
	dma_parameter_struct dma_init_struct;
	rcu_periph_clock_enable(RCU_DMA0);
	rcu_periph_clock_enable(RCU_DMA1);

#if defined (DMX_USE_USART0)
	dma_deinit(USART0_DMA, USART0_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t) s_TxBuffer[config::USART0_PORT].data;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = s_TxBuffer[config::USART0_PORT].nLength;
	dma_init_struct.periph_addr = USART0 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART0_DMA, USART0_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART0_DMA, USART0_TX_DMA_CH);
	dma_memory_to_memory_disable(USART0_DMA, USART0_TX_DMA_CH);
#endif
#if defined (DMX_USE_USART1)
	dma_deinit(USART1_DMA, USART1_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t) s_TxBuffer[config::USART1_PORT].data;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = s_TxBuffer[config::USART1_PORT].nLength;
	dma_init_struct.periph_addr = USART1 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART1_DMA, USART1_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART1_DMA, USART1_TX_DMA_CH);
	dma_memory_to_memory_disable(USART1_DMA, USART1_TX_DMA_CH);
#endif
#if defined (DMX_USE_USART2)
	dma_deinit(USART2_DMA, USART2_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t) s_TxBuffer[config::USART2_PORT].data;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = s_TxBuffer[config::USART2_PORT].nLength;
	dma_init_struct.periph_addr = USART2 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART2_DMA, USART2_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART2_DMA, USART2_TX_DMA_CH);
	dma_memory_to_memory_disable(USART2_DMA, USART2_TX_DMA_CH);
#endif
#if defined (DMX_USE_UART3)
	dma_deinit(DMA1, DMA_CH4);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t) s_TxBuffer[config::UART3_PORT].data;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = s_TxBuffer[config::UART3_PORT].nLength;
	dma_init_struct.periph_addr = UART3 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(DMA1, DMA_CH4, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(DMA1, DMA_CH4);
	dma_memory_to_memory_disable(DMA1, DMA_CH4);
#endif
#if defined (DMX_USE_UART4)
	dma_deinit(DMA1, DMA_CH3);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t) s_TxBuffer[config::UART4_PORT].data;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = s_TxBuffer[config::UART4_PORT].nLength;
	dma_init_struct.periph_addr = UART4 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(DMA1, DMA_CH3, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(DMA1, DMA_CH3);
	dma_memory_to_memory_disable(DMA1, DMA_CH3);
#endif
#if defined (DMX_USE_USART5)
	dma_deinit(USART5_DMA, USART5_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t) s_TxBuffer[config::USART5_PORT].data;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = s_TxBuffer[config::USART5_PORT].nLength;
	dma_init_struct.periph_addr = USART5 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART5_DMA, USART5_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART5_DMA, USART5_TX_DMA_CH);
	dma_memory_to_memory_disable(USART5_DMA, USART5_TX_DMA_CH);
#endif
#if defined (DMX_USE_UART6)
	dma_deinit(UART6_DMA, UART6_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t) s_TxBuffer[config::UART6_PORT].data;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = s_TxBuffer[config::UART6_PORT].nLength;
	dma_init_struct.periph_addr = UART6 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART6_DMA, UART6_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART6_DMA, UART6_TX_DMA_CH);
	dma_memory_to_memory_disable(UART6_DMA, UART6_TX_DMA_CH);
#endif
#if defined (DMX_USE_UART7)
	dma_deinit(UART7_DMA, UART7_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t) s_TxBuffer[config::UART7_PORT].data;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = s_TxBuffer[config::UART7_PORT].nLength;
	dma_init_struct.periph_addr = UART7 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART7_DMA, UART7_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART7_DMA, UART7_TX_DMA_CH);
	dma_memory_to_memory_disable(UART7_DMA, UART7_TX_DMA_CH);
#endif
}

extern "C" {
void TIMER1_IRQHandler() {
	__DMB();
	const auto nIntFlag = TIMER_INTF(TIMER1);
	const auto nUartsSending = s_nUartsSending;
	PE1_HIGH();

	if ((nIntFlag & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {		// Start break
#if defined (DMX_USE_USART0)
		gpio_init(USART0_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USART0_TX_PIN);
		if (nUartsSending & (1U << config::USART0_PORT)) {
			s_nUartsSending |= (1U << (config::USART0_PORT + 8));
			GPIO_BC(USART0_GPIO_PORT) = USART0_TX_PIN;
		}
#endif
#if defined (DMX_USE_USART1)
		gpio_init(USART1_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN);
		if (nUartsSending & (1U << config::USART1_PORT)) {
			s_nUartsSending |= (1U << (config::USART1_PORT + 8));
			GPIO_BC(USART1_GPIO_PORT) = USART1_TX_PIN;
		}
#endif
#if defined (DMX_USE_USART2)
		gpio_init(USART2_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USART2_TX_PIN);
		if (nUartsSending & (1U << config::USART2_PORT)) {
			s_nUartsSending |= (1U << (config::USART2_PORT + 8));
			GPIO_BC(USART2_GPIO_PORT) = USART2_TX_PIN;
		}
#endif
#if defined (DMX_USE_UART3)
		gpio_init(UART3_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART3_TX_PIN);
		if (nUartsSending & (1U << config::UART3_PORT)) {
			s_nUartsSending |= (1U << (config::UART3_PORT + 8));
			GPIO_BC(UART3_GPIO_PORT) = UART3_TX_PIN;
		}
#endif
#if defined (DMX_USE_UART4)
		gpio_init(UART4_GPIO_TX_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART4_TX_PIN);
		if (nUartsSending & (1U << config::UART4_PORT)) {
			s_nUartsSending |= (1U << (config::UART4_PORT + 8));
			GPIO_BC(UART4_GPIO_TX_PORT) = UART4_TX_PIN;
		}
#endif
#if defined (DMX_USE_USART5)
		gpio_init(USART5_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USART5_TX_PIN);
		if (nUartsSending & (1U << config::USART5_PORT)) {
			s_nUartsSending |= (1U << (config::USART5_PORT + 8));
			GPIO_BC(USART5_GPIO_PORT) = USART5_TX_PIN;
		}
#endif
#if defined (DMX_USE_UART6)
		gpio_init(UART6_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART6_TX_PIN);
		if (nUartsSending & (1U << config::UART6_PORT)) {
			s_nUartsSending |= (1U << (config::UART6_PORT + 8));
			GPIO_BC(UART6_GPIO_PORT) = UART6_TX_PIN;
		}
#endif
#if defined (DMX_USE_UART7)
		gpio_init(UART7_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART7_TX_PIN);
		if (nUartsSending & (1U << config::UART7_PORT)) {
			s_nUartsSending |= (1U << (config::UART7_PORT + 8));
			GPIO_BC(UART7_GPIO_PORT) = UART7_TX_PIN;
		}
#endif
	} else if ((nIntFlag & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {	// Stop break
#if defined (DMX_USE_USART0)
		if (nUartsSending & (1U << (config::USART0_PORT + 8))) {
			gpio_init(USART0_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART0_TX_PIN);
		}
#endif
#if defined (DMX_USE_USART1)
		if (nUartsSending & (1U << (config::USART1_PORT + 8))) {
			gpio_init(USART1_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN);
		}
#endif
#if defined (DMX_USE_USART2)
		if (nUartsSending & (1U << (config::USART2_PORT + 8))) {
			gpio_init(USART2_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART2_TX_PIN);
		}
#endif
#if defined (DMX_USE_UART3)
		if (nUartsSending & (1U << (config::UART3_PORT + 8))) {
			gpio_init(UART3_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ,  UART3_TX_PIN);
		}
#endif
#if defined (DMX_USE_UART4)
		if (nUartsSending & (1U << (config::UART4_PORT + 8))) {
			gpio_init(UART4_GPIO_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART4_TX_PIN);
		}
#endif
#if defined (DMX_USE_USART5)
		if (nUartsSending & (1U << (config::USART5_PORT + 8))) {
			gpio_init(USART5_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART5_TX_PIN);
		}
#endif
#if defined (DMX_USE_UART6)
		if (nUartsSending & (1U << (config::UART6_PORT + 8))) {
			gpio_init(UART6_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART6_TX_PIN);
		}
#endif
#if defined (DMX_USE_UART7)
		if (nUartsSending & (1U << (config::UART7_PORT + 8))) {
			gpio_init(UART7_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART7_TX_PIN);
		}
#endif
	} else if ((nIntFlag & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {	// Send data
#if defined (DMX_USE_USART0)
		if (nUartsSending & (1U << (config::USART0_PORT + 8))) {
			DMA_CHCTL(USART0_DMA, USART0_TX_DMA_CH) &= ~DMA_CHXCTL_CHEN;
			DMA_CHMADDR(USART0_DMA, USART0_TX_DMA_CH) = (uint32_t) s_TxBuffer[config::USART0_PORT].data;
			DMA_CHCNT(USART0_DMA, USART0_TX_DMA_CH) = (s_TxBuffer[config::USART0_PORT].nLength & DMA_CHANNEL_CNT_MASK);
			DMA_CHCTL(USART0_DMA, USART0_TX_DMA_CH) |= DMA_CHXCTL_CHEN;
			usart_dma_transmit_config(USART0, USART_DENT_ENABLE);
		}
#endif
#if defined (DMX_USE_USART1)
		if (nUartsSending & (1U << (config::USART1_PORT + 8))) {
			DMA_CHCTL(USART1_DMA, USART1_TX_DMA_CH) &= ~DMA_CHXCTL_CHEN;
			DMA_CHMADDR(USART1_DMA, USART1_TX_DMA_CH) = (uint32_t) s_TxBuffer[config::USART1_PORT].data;
			DMA_CHCNT(USART1_DMA, USART1_TX_DMA_CH) = (s_TxBuffer[config::USART1_PORT].nLength & DMA_CHANNEL_CNT_MASK);
			DMA_CHCTL(USART1_DMA, USART1_TX_DMA_CH) |= DMA_CHXCTL_CHEN;
			usart_dma_transmit_config(USART1, USART_DENT_ENABLE);
		}
#endif
#if defined (DMX_USE_USART2)
		if (nUartsSending & (1U << (config::USART2_PORT + 8))) {
			DMA_CHCTL(USART2_DMA, USART2_TX_DMA_CH) &= ~DMA_CHXCTL_CHEN;
			DMA_CHMADDR(USART2_DMA, USART2_TX_DMA_CH) = (uint32_t) s_TxBuffer[config::USART2_PORT].data;
			DMA_CHCNT(USART2_DMA, USART2_TX_DMA_CH) = (s_TxBuffer[config::USART2_PORT].nLength & DMA_CHANNEL_CNT_MASK);
			DMA_CHCTL(USART2_DMA, USART2_TX_DMA_CH) |= DMA_CHXCTL_CHEN;
			usart_dma_transmit_config(USART2, USART_DENT_ENABLE);
		}
#endif
#if defined (DMX_USE_UART3)
		if (nUartsSending & (1U << (config::UART3_PORT + 8))) {
			dma_channel_disable(UART3_DMA, UART3_TX_DMA_CH);
			DMA_CHMADDR(UART3_DMA, UART3_TX_DMA_CH) = (uint32_t) s_TxBuffer[config::UART3_PORT].data;
			DMA_CHCNT(UART3_DMA, UART3_TX_DMA_CH) = (s_TxBuffer[config::UART3_PORT].nLength & DMA_CHANNEL_CNT_MASK);
			dma_channel_enable(UART3_DMA, UART3_TX_DMA_CH);
			usart_dma_transmit_config(UART3, USART_DENT_ENABLE);
		}
#endif
#if defined (DMX_USE_UART4)
		if (nUartsSending & (1U << (config::UART4_PORT + 8))) {
			dma_channel_disable(UART4_DMA, UART4_TX_DMA_CH);
			DMA_CHMADDR(UART4_DMA, UART4_TX_DMA_CH) = (uint32_t) s_TxBuffer[config::UART4_PORT].data;
			DMA_CHCNT(UART4_DMA, UART4_TX_DMA_CH) = (s_TxBuffer[config::UART4_PORT].nLength & DMA_CHANNEL_CNT_MASK);
			dma_channel_enable(UART4_DMA, UART4_TX_DMA_CH);
			usart_dma_transmit_config(UART4, USART_DENT_ENABLE);
		}
#endif
#if defined (DMX_USE_USART5)
		if (nUartsSending & (1U << (config::USART5_PORT + 8))) {
			DMA_CHCTL(USART5_DMA, USART5_TX_DMA_CH) &= ~DMA_CHXCTL_CHEN;
			DMA_CHMADDR(USART5_DMA, USART5_TX_DMA_CH) = (uint32_t) s_TxBuffer[config::USART5_PORT].data;
			DMA_CHCNT(USART5_DMA, USART5_TX_DMA_CH) = (s_TxBuffer[config::USART5_PORT].nLength & DMA_CHANNEL_CNT_MASK);
			DMA_CHCTL(USART5_DMA, USART5_TX_DMA_CH) |= DMA_CHXCTL_CHEN;
			usart_dma_transmit_config(USART5, USART_DENT_ENABLE);
		}
#endif
#if defined (DMX_USE_UART6)
		if (nUartsSending & (1U << (config::UART6_PORT + 8))) {
			dma_channel_disable(UART6_DMA, UART6_TX_DMA_CH);
			DMA_CHMADDR(UART6_DMA, UART6_TX_DMA_CH) = (uint32_t) s_TxBuffer[config::UART6_PORT].data;
			DMA_CHCNT(UART6_DMA, UART6_TX_DMA_CH) = (s_TxBuffer[config::UART6_PORT].nLength & DMA_CHANNEL_CNT_MASK);
			dma_channel_enable(UART6_DMA, UART6_TX_DMA_CH);
			usart_dma_transmit_config(UART6, USART_DENT_ENABLE);
		}
#endif
#if defined (DMX_USE_UART7)
		if (nUartsSending & (1U << (config::UART7_PORT + 8))) {
			dma_channel_disable(UART7_DMA, UART7_TX_DMA_CH);
			DMA_CHMADDR(UART7_DMA, UART7_TX_DMA_CH) = (uint32_t) s_TxBuffer[config::UART6_PORT].data;
			DMA_CHCNT(UART7_DMA, UART7_TX_DMA_CH) = (s_TxBuffer[config::UART7_PORT].nLength & DMA_CHANNEL_CNT_MASK);
			dma_channel_enable(UART7_DMA, UART7_TX_DMA_CH);
			usart_dma_transmit_config(UART7, USART_DENT_ENABLE);
		}
#endif
	} else {
		// 0x00000013
	}

	timer_interrupt_flag_clear(TIMER1, nIntFlag);

	PE1_LOW();
	__DMB();
}

void TIMER2_IRQHandler() {
	__DMB();
	const auto nIntFlag = TIMER_INTF(TIMER2);

	if ((nIntFlag & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
		if (s_RxBuffer[0].State == TxRxState::DMXDATA) {
			s_RxBuffer[0].State = TxRxState::IDLE;
			s_RxBuffer[0].Dmx.nSlotsInPacket |= 0x8000;
			PE1_LOW();
		}
	}
#if DMX_MAX_PORTS >= 2
	else if ((nIntFlag & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
		if (s_RxBuffer[1].State == TxRxState::DMXDATA) {
			s_RxBuffer[1].State = TxRxState::IDLE;
			s_RxBuffer[1].Dmx.nSlotsInPacket |= 0x8000;
		}
	}
#endif
#if DMX_MAX_PORTS >= 3
	else if ((nIntFlag & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
		if (s_RxBuffer[2].State == TxRxState::DMXDATA) {
			s_RxBuffer[2].State = TxRxState::IDLE;
			s_RxBuffer[2].Dmx.nSlotsInPacket |= 0x8000;
		}
	}
#endif
#if DMX_MAX_PORTS >= 4
	else if ((nIntFlag & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
		if (s_RxBuffer[3].State == TxRxState::DMXDATA) {
			s_RxBuffer[3].State = TxRxState::IDLE;
			s_RxBuffer[3].Dmx.nSlotsInPacket |= 0x8000;
		}
	}
#endif
	timer_interrupt_flag_clear(TIMER2, nIntFlag);
	__DMB();
}

void TIMER3_IRQHandler() {
	__DMB();
	const auto nIntFlag = TIMER_INTF(TIMER3);
#if DMX_MAX_PORTS >= 5
	if ((nIntFlag & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
		if (s_RxBuffer[4].State == TxRxState::DMXDATA) {
			s_RxBuffer[4].State = TxRxState::IDLE;
			s_RxBuffer[4].Dmx.nSlotsInPacket |= 0x8000;

			PE1_LOW();
		}
	}
# if DMX_MAX_PORTS >= 6
	else if ((nIntFlag & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
		if (s_RxBuffer[5].State == TxRxState::DMXDATA) {
			s_RxBuffer[5].State = TxRxState::IDLE;
			s_RxBuffer[5].Dmx.nSlotsInPacket |= 0x8000;
		}
	}
# endif
# if DMX_MAX_PORTS >= 7
	else if ((nIntFlag & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
		if (s_RxBuffer[6].State == TxRxState::DMXDATA) {
			s_RxBuffer[6].State = TxRxState::IDLE;
			s_RxBuffer[6].Dmx.nSlotsInPacket |= 0x8000;
		}
	}
# endif
# if DMX_MAX_PORTS == 8
	else if ((nIntFlag & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
		if (s_RxBuffer[7].State == TxRxState::DMXDATA) {
			s_RxBuffer[7].State = TxRxState::IDLE;
			s_RxBuffer[7].Dmx.nSlotsInPacket |= 0x8000;
		}
	}
# endif
#endif
	timer_interrupt_flag_clear(TIMER3, nIntFlag);
	__DMB();
}

void TIMER6_IRQHandler() {
	 __DMB();

	for (uint32_t i = 0; i < DMX_MAX_PORTS; i++) {
		sv_nRxDmxPackets[i].nPerSecond = sv_nRxDmxPackets[i].nCount - sv_nRxDmxPackets[i].nCountPrevious;
		sv_nRxDmxPackets[i].nCountPrevious = sv_nRxDmxPackets[i].nCount;
	}

	timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
	 __DMB();
}
}

Dmx *Dmx::s_pThis = nullptr;

Dmx::Dmx() {
	DEBUG_ENTRY
	assert(s_pThis == nullptr);
	s_pThis = this;

	s_nUartsSending = 0;

	for (uint32_t i = 0; i < DMX_MAX_PORTS; i++) {
		ClearData(i);
		sv_PortState[i] = PortState::IDLE;
		SetPortDirection(i, PortDirection::INP, false);
		s_RxBuffer[i].State = TxRxState::IDLE;
	}

	gpio_config();		// DEBUG Logic Analyzer
	usart_dma_config();	// DMX Transmit
	timer2_config();	// DMX Receive	-> Slot time-out Port 0,1,2,3
	timer3_config();	// DMX Receive	-> Slot time-out Port 4,5,6,7
	timer6_config();	// DMX Receive	-> Updates per second

#if defined (DMX_USE_USART0)
	uart_config(USART0);
	NVIC_EnableIRQ(USART0_IRQn);
#endif
#if defined (DMX_USE_USART1)
	uart_config(USART1);
	NVIC_EnableIRQ(USART1_IRQn);
#endif
#if defined (DMX_USE_USART2)
	uart_config(USART2);
	NVIC_EnableIRQ(USART2_IRQn);
#endif
#if defined (DMX_USE_UART3)
	uart_config(UART3);
	NVIC_EnableIRQ(UART3_IRQn);
#endif
#if defined (DMX_USE_UART4)
	uart_config(UART4);
	NVIC_EnableIRQ(UART4_IRQn);
#endif
#if defined (DMX_USE_USART5)
	uart_config(USART5);
	NVIC_EnableIRQ(USART5_IRQn);
#endif
#if defined (DMX_USE_UART6)
	uart_config(UART6);
	NVIC_EnableIRQ(UART6_IRQn);
#endif
#if defined (DMX_USE_UART7)
	uart_config(UART7);
	NVIC_EnableIRQ(UART7_IRQn);
#endif

	DEBUG_EXIT
}

void Dmx::SetPortDirection(uint32_t nPort, PortDirection tPortDirection, bool bEnableData) {
	assert(nPort < DMX_MAX_PORTS);
	const auto nUart = _port_to_uart(nPort);

	if (m_tDmxPortDirection[nPort] != tPortDirection) {
		m_tDmxPortDirection[nPort] = tPortDirection;
		StopData(nUart, nPort);
		if (tPortDirection == PortDirection::OUTP) {
			GPIO_BOP(s_DirGpio[nPort].nPort) = s_DirGpio[nPort].nPin;
		} else if (tPortDirection == PortDirection::INP) {
			GPIO_BC(s_DirGpio[nPort].nPort) = s_DirGpio[nPort].nPin;
		} else {
			assert(0);
		}
	} else if (!bEnableData) {
		StopData(nUart, nPort);
	}

	if (bEnableData) {
		StartData(nUart, nPort);
	}
}

void Dmx::ClearData(uint32_t nPort) {
	assert(nPort < DMX_MAX_PORTS);
	auto *p = &s_TxBuffer[nPort];
	auto *p16 = reinterpret_cast<uint16_t *>(p->data);

	for (uint32_t i = 0; i < 514 / 2; i++) {
		*p16++ = 0;
	}

	p->nLength = 513; // Including START Code
}

void Dmx::StartData(uint32_t nUart, uint32_t nPort) {
	assert(nPort < DMX_MAX_PORTS);
	assert(sv_PortState[nPort] == PortState::IDLE);

	if (m_tDmxPortDirection[nPort] == PortDirection::OUTP) {
		if (s_nUartsSending == 0) {
			timer1_config();
		}
		s_nUartsSending |= (1U << nPort);
		sv_PortState[nPort] = PortState::TX;
	} else if (m_tDmxPortDirection[nPort] == PortDirection::INP) {
		s_RxBuffer[nPort].State = TxRxState::IDLE;

		while (RESET == usart_flag_get(nUart, USART_FLAG_TBE))
			;

		usart_interrupt_flag_clear(nUart, ~0);
	    usart_interrupt_enable(nUart, USART_INT_RBNE);

	    sv_PortState[nPort] = PortState::RX;

		switch (nPort) {
		case 0:
			TIMER_DMAINTEN(TIMER2) |= (uint32_t) TIMER_INT_CH0;
			break;
#if DMX_MAX_PORTS >= 2
		case 1:
			TIMER_DMAINTEN(TIMER2) |= (uint32_t) TIMER_INT_CH1;
			break;
#endif
#if DMX_MAX_PORTS >= 3
		case 2:
			TIMER_DMAINTEN(TIMER2) |= (uint32_t) TIMER_INT_CH2;
			break;
#endif
#if DMX_MAX_PORTS >= 4
		case 3:
			TIMER_DMAINTEN(TIMER2) |= (uint32_t) TIMER_INT_CH3;
			break;
#endif
#if DMX_MAX_PORTS >= 5
		case 4:
			TIMER_DMAINTEN(TIMER3) |= (uint32_t) TIMER_INT_CH0;
			break;
#endif
#if DMX_MAX_PORTS >= 6
		case 5:
			TIMER_DMAINTEN(TIMER3) |= (uint32_t) TIMER_INT_CH1;
			break;
#endif
#if DMX_MAX_PORTS >= 7
		case 6:
			TIMER_DMAINTEN(TIMER3) |= (uint32_t) TIMER_INT_CH2;
			break;
#endif
#if DMX_MAX_PORTS == 8
		case 7:
			TIMER_DMAINTEN(TIMER3) |= (uint32_t) TIMER_INT_CH3;
			break;
#endif
		default:
			assert(0);
			__builtin_unreachable();
			break;
		}
	} else {
		assert(0);
		__builtin_unreachable();
	}
}

void Dmx::StopData(uint32_t nUart, uint32_t nPort) {
	assert(nPort < DMX_MAX_PORTS);

	if (sv_PortState[nPort] == PortState::IDLE) {
		return;
	}

	if (m_tDmxPortDirection[nPort] == PortDirection::OUTP) {
		s_nUartsSending &= ((1U << nPort) | (1U << (nPort + 8)));
	} else if (m_tDmxPortDirection[nPort] == PortDirection::INP) {
		usart_interrupt_disable(nUart, USART_INT_RBNE);
		s_RxBuffer[nPort].State = TxRxState::IDLE;

		switch (nPort) {
		case 0:
			TIMER_DMAINTEN(TIMER2) &= (~(uint32_t) TIMER_INT_CH0);
			break;
#if DMX_MAX_PORTS >= 2
		case 1:
			TIMER_DMAINTEN(TIMER2) &= (~(uint32_t) TIMER_INT_CH1);
			break;
#endif
#if DMX_MAX_PORTS >= 3
		case 2:
			TIMER_DMAINTEN(TIMER2) &= (~(uint32_t) TIMER_INT_CH2);
			break;
#endif
#if DMX_MAX_PORTS >= 4
		case 3:
			TIMER_DMAINTEN(TIMER2) &= (~(uint32_t) TIMER_INT_CH3);
			break;
#endif
#if DMX_MAX_PORTS >= 5
		case 4:
			TIMER_DMAINTEN(TIMER3) &= (~(uint32_t) TIMER_INT_CH0);
			break;
#endif
#if DMX_MAX_PORTS >= 6
		case 5:
			TIMER_DMAINTEN(TIMER3) &= (~(uint32_t) TIMER_INT_CH1);
			break;
#endif
#if DMX_MAX_PORTS >= 7
		case 6:
			TIMER_DMAINTEN(TIMER3) &= (~(uint32_t) TIMER_INT_CH2);
			break;
#endif
#if DMX_MAX_PORTS == 8
		case 7:
			TIMER_DMAINTEN(TIMER3) &= (~(uint32_t) TIMER_INT_CH3);
			break;
#endif
		default:
			assert(0);
			__builtin_unreachable();
			break;
		}
	} else {
		assert(0);
		__builtin_unreachable();
	}

	sv_PortState[nPort] = PortState::IDLE;
}

// DMX Send

void Dmx::SetDmxBreakTime(uint32_t nBreakTime) {
	s_nDmxTransmitBreakTime = std::max(transmit::BREAK_TIME_MIN, nBreakTime);

	SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
}

void Dmx::SetDmxMabTime(uint32_t nMabTime) {
	s_nDmxTransmitMabTime = std::max(transmit::MAB_TIME_MIN, nMabTime);

	SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
}

void Dmx::SetDmxPeriodTime(uint32_t nPeriod) {
	m_nDmxTransmitPeriodRequested = nPeriod;

	auto nLengthMax = s_TxBuffer[0].nLength;

	for (uint32_t i = 1; i < 2; i++) { // TODO
		if (s_TxBuffer[i].nLength > nLengthMax) {
			nLengthMax = s_TxBuffer[i].nLength;
		}
	}

	const auto nPackageLengthMicroSeconds = s_nDmxTransmitBreakTime + s_nDmxTransmitMabTime + (nLengthMax * 44) + 44;

	if (nPeriod != 0) {
		if (nPeriod < nPackageLengthMicroSeconds) {
			s_nDmxTransmitPeriod = std::max(transmit::BREAK_TO_BREAK_TIME_MIN, nPackageLengthMicroSeconds + 44U);
		} else {
			s_nDmxTransmitPeriod = nPeriod;
		}
	} else {
		s_nDmxTransmitPeriod =  std::max(transmit::BREAK_TO_BREAK_TIME_MIN, nPackageLengthMicroSeconds + 44U);
	}

	DEBUG_PRINTF("nPeriod=%u, nLengthMax=%u, s_nDmxTransmitPeriod=%u", nPeriod, nLengthMax, s_nDmxTransmitPeriod);
}

void Dmx::SetDmxSlots(uint16_t nSlots) {
	if ((nSlots >= 2) && (nSlots <= dmx::max::CHANNELS)) {
		m_nDmxTransmitSlots = nSlots;

		for (uint32_t i = 0; i < dmxmulti::config::max::OUT; i++) {
			if (m_nDmxTransmissionLength[i] != 0) {
				m_nDmxTransmissionLength[i] = std::min(m_nDmxTransmissionLength[i], static_cast<uint32_t>(nSlots));
			}
		}

		SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
	}
}

void Dmx::SetPortSendDataWithoutSC(uint32_t nPort, const uint8_t *pData, uint32_t nLength) {
	assert(nPort < DMX_MAX_PORTS);

	auto *p = &s_TxBuffer[nPort];

	auto *pDst = p->data;
	nLength = std::min(nLength, static_cast<uint32_t>(m_nDmxTransmitSlots));
	p->nLength = nLength + 1U;

	__builtin_prefetch(pData);
	memcpy(&pDst[1], pData,  nLength);

	if (nLength != m_nDmxTransmissionLength[nPort]) {
		m_nDmxTransmissionLength[nPort] = nLength;
		SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
	}
}

// DMX Receive

#include <cstdio>

const uint8_t *Dmx::GetDmxAvailable(uint32_t nPort)  {
	assert(nPort < DMX_MAX_PORTS);

	if ((s_RxBuffer[nPort].Dmx.nSlotsInPacket & 0x8000) != 0x8000) {
		return nullptr;
	}

	s_RxBuffer[nPort].Dmx.nSlotsInPacket &= ~0x8000;
	s_RxBuffer[nPort].Dmx.nSlotsInPacket--;	// Remove SC from length

	return s_RxBuffer[nPort].data;
}

uint32_t Dmx::GetUpdatesPerSecond(uint32_t nPort) {
	assert(nPort < DMX_MAX_PORTS);
	__DMB();
	return sv_nRxDmxPackets[nPort].nPerSecond;
}

// RDM Send

void Dmx::RdmSendRaw(uint32_t nPort, const uint8_t* pRdmData, uint32_t nLength) {
	assert(nPort < DMX_MAX_PORTS);
	assert(pRdmData != nullptr);
	assert(nLength != 0);

	const auto nUart = _port_to_uart(nPort);

	while (RESET == usart_flag_get(nUart, USART_FLAG_TC))
		;

	switch (nUart) {
#if defined (DMX_USE_USART0)
		case USART0:
			gpio_init(USART0_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USART0_TX_PIN);
			GPIO_BC(USART0_GPIO_PORT) = USART0_TX_PIN;
			break;
#endif
#if defined (DMX_USE_USART1)
		case USART1:
			gpio_init(USART1_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN);
			GPIO_BC(USART1_GPIO_PORT) = USART1_TX_PIN;
			break;
#endif
#if defined (DMX_USE_USART2)
		case USART2:
			gpio_init(USART2_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USART2_TX_PIN);
			GPIO_BC(USART2_GPIO_PORT) = USART2_TX_PIN;
			break;
#endif
#if defined (DMX_USE_UART3)
		case UART3:
			gpio_init(UART3_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART3_TX_PIN);
			GPIO_BC(UART3_GPIO_PORT) = UART3_TX_PIN;
			break;
#endif
#if defined (DMX_USE_UART4)
		case UART4:
			gpio_init(UART4_GPIO_TX_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART4_TX_PIN);
			GPIO_BC(UART4_GPIO_TX_PORT) = UART4_TX_PIN;
			break;
#endif
#if defined (DMX_USE_USART5)
		case USART5:
			gpio_init(USART5_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USART5_TX_PIN);
			GPIO_BC(USART5_GPIO_PORT) = USART5_TX_PIN;
			break;
#endif
#if defined (DMX_USE_UART6)
		case UART6:
			gpio_init(UART6_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART6_TX_PIN);
			GPIO_BC(UART6_GPIO_PORT) = UART6_TX_PIN;
			break;
#endif
#if defined (DMX_USE_UART7)
		case UART7:
			gpio_init(UART7_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART7_TX_PIN);
			GPIO_BC(UART7_GPIO_PORT) = UART7_TX_PIN;
			break;
#endif
		default:
			assert(0);
			__builtin_unreachable();
			break;
	}

	udelay(RDM_TRANSMIT_BREAK_TIME);

	switch (nUart) {
#if defined (DMX_USE_USART0)
		case USART0:
			gpio_init(USART0_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART0_TX_PIN);
			break;
#endif
#if defined (DMX_USE_USART1)
		case USART1:
			gpio_init(USART1_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN);
			break;
#endif
#if defined (DMX_USE_USART2)
		case USART2:
			gpio_init(USART2_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART2_TX_PIN);
			break;
#endif
#if defined (DMX_USE_UART3)
		case UART3:
			gpio_init(USART5_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART3_TX_PIN);
			break;
#endif
#if defined (DMX_USE_UART4)
		case UART4:
			gpio_init(UART4_GPIO_TX_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART4_TX_PIN);
			break;
#endif
#if defined (DMX_USE_USART5)
		case USART5:
			gpio_init(USART5_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART5_TX_PIN);
			break;
#endif
#if defined (DMX_USE_UART6)
		case UART6:
			gpio_init(UART6_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART6_TX_PIN);
			break;
#endif
#if defined (DMX_USE_UART7)
		case UART7:
			gpio_init(UART7_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, UART7_TX_PIN);
			break;
#endif
		default:
			assert(0);
			__builtin_unreachable();
			break;
	}

	udelay(RDM_TRANSMIT_MAB_TIME);

	for (uint32_t i = 0; i < nLength; i++) {
		while (RESET == usart_flag_get(nUart, USART_FLAG_TBE))
			;
		USART_DATA(nUart) = static_cast<uint16_t>(USART_DATA_DATA & pRdmData[i]);
	}

	while (SET == usart_flag_get(nUart, USART_FLAG_BSY)) {
		static_cast<void>(GET_BITS(USART_DATA(nUart), 0U, 8U));
	}

	DEBUG_EXIT
}

// RDM Receive

const uint8_t *Dmx::RdmReceive(uint32_t nPort) {
	assert(nPort < DMX_MAX_PORTS);

	if ((s_RxBuffer[nPort].Rdm.nIndex & 0x4000) != 0x4000) {
		return nullptr;
	}

	s_RxBuffer[nPort].Dmx.nSlotsInPacket = 0; // This is correct.
	return s_RxBuffer[nPort].data;
}

const uint8_t *Dmx::RdmReceiveTimeOut(uint32_t nPort, uint16_t nTimeOut) {
	assert(nPort < DMX_MAX_PORTS);

	uint8_t *p = nullptr;
	TIMER_CNT(TIMER5) = 0;

	do {
		if ((p = const_cast<uint8_t*>(RdmReceive(nPort))) != nullptr) {
			return p;
		}
	} while (TIMER_CNT(TIMER5) < nTimeOut);

	return nullptr;
}

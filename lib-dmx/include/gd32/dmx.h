/**
 * @file dmx.h
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

#ifndef GD32_DMX_H_
#define GD32_DMX_H_

#include <cstdint>

#include "dmxconst.h"
#include "dmx_config.h"

struct Statistics {
	uint32_t nSlotsInPacket;
	uint32_t nSlotToSlot;
};

struct Data {
	uint8_t data[dmx::buffer::SIZE];
	struct Statistics Statistics;
};

class Dmx {
public:
	Dmx();

	void SetPortDirection(uint32_t nPort, dmx::PortDirection portDirection, bool bEnableData = false);

	// RDM Send

	void RdmSendRaw(uint32_t nPort, const uint8_t *pRdmData, uint32_t nLength);

	// RDM Receive

	const uint8_t *RdmReceive(uint32_t nPort);
	const uint8_t *RdmReceiveTimeOut(uint32_t nPort, uint16_t nTimeOut);

	 uint32_t RdmGetDateReceivedEnd() {
		 return 0;
	 }

	// DMX Send

	void SetPortSendDataWithoutSC(uint32_t nPort, const uint8_t *pData, uint32_t nLength);

	void SetDmxBreakTime(uint32_t nBreakTime);
	uint32_t GetDmxBreakTime() const {
		return m_nDmxTransmitMabTime;
	}

	void SetDmxMabTime(uint32_t nMabTime);
	uint32_t GetDmxMabTime() const {
		return m_nDmxTransmitMabTime;
	}

	void SetDmxPeriodTime(uint32_t nPeriod);
	uint32_t GetDmxPeriodTime() const {
		return m_nDmxTransmitPeriod;
	}

	void SetDmxSlots(uint16_t nSlots = dmx::max::CHANNELS);
	uint16_t GetDmxSlots() const {
		return m_nDmxTransmitSlots;
	}

	// DMX Receive

	const uint8_t* GetDmxAvailable(uint32_t nPort = 0);
	uint32_t GetUpdatesPerSecond(uint32_t nPort = 0);

	static Dmx* Get() {
		return s_pThis;
	}

private:
	void ClearData(uint32_t uart);
	void StartData(uint32_t nUart, uint32_t nPort);
	void StopData(uint32_t nUart, uint32_t nPort);

private:
	uint32_t m_nDmxTransmitBreakTime { dmx::transmit::BREAK_TIME_MIN };
	uint32_t m_nDmxTransmitMabTime { dmx::transmit::MAB_TIME_MIN };
	uint32_t m_nDmxTransmitPeriod { dmx::transmit::PERIOD_DEFAULT };
	uint32_t m_nDmxTransmitPeriodRequested { dmx::transmit::PERIOD_DEFAULT };
	uint16_t m_nDmxTransmitSlots { dmx::max::CHANNELS };
	dmx::PortDirection m_tDmxPortDirection[dmxmulti::config::max::OUT];
	uint32_t m_nDmxTransmissionLength[dmxmulti::config::max::OUT];

	static Dmx *s_pThis;
};

#endif /* GD32_DMX_H_ */

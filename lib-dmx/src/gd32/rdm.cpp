/**
 * @file rdm.cpp
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

#include <cstdint>
#include <cassert>

#include "rdm.h"
#include "dmxconst.h"

#include "debug.h"

#include "dmx_internal.h"

#include "dmxmulti.h"

using namespace dmxmulti;

static uint8_t m_TransactionNumber[8];

void Rdm::Send(uint32_t nPort, struct TRdmMessage *pRdmCommand, uint32_t nSpacingMicros) {
	DEBUG_PRINTF("nPort=%u, pRdmData=%p, nSpacingMicros=%u", nPort, pRdmCommand, nSpacingMicros);

	assert(nPort < config::MAX_PORTS);
	assert(pRdmCommand != nullptr);

#if 0
	if (nSpacingMicros != 0) {
		const auto nMicros = H3_TIMER->AVS_CNT1;
		const auto nDeltaMicros = nMicros - m_nLastSendMicros[nPort];
		if (nDeltaMicros < nSpacingMicros) {
			const auto nWait = nSpacingMicros - nDeltaMicros;
			do {
			} while ((H3_TIMER->AVS_CNT1 - nMicros) < nWait);
		}
	}

	m_nLastSendMicros[nPort] = H3_TIMER->AVS_CNT1;
#endif

	auto *rdm_data = reinterpret_cast<uint8_t*>(pRdmCommand);
	uint32_t i;
	uint16_t rdm_checksum = 0;

	pRdmCommand->transaction_number = m_TransactionNumber[nPort];

	for (i = 0; i < pRdmCommand->message_length; i++) {
		rdm_checksum = static_cast<uint16_t>(rdm_checksum + rdm_data[i]);
	}

	rdm_data[i++] = static_cast<uint8_t>(rdm_checksum >> 8);
	rdm_data[i] = static_cast<uint8_t>(rdm_checksum & 0XFF);

	SendRaw(nPort, reinterpret_cast<const uint8_t*>(pRdmCommand), pRdmCommand->message_length + RDM_MESSAGE_CHECKSUM_SIZE);

	m_TransactionNumber[nPort]++;

	DEBUG_EXIT
}

void Rdm::SendRawRespondMessage(uint32_t nPort, const uint8_t *pRdmData, uint32_t nLength) {
	DEBUG_PRINTF("nPort=%u, pRdmData=%p, nLength=%u", nPort, pRdmData, nLength);

	SendRaw(nPort, pRdmData, nLength);
}

void Rdm::SendDiscoveryRespondMessage(uint32_t nPort, const uint8_t *pRdmData, uint32_t nLength) {
	DEBUG_PRINTF("nPort=%u, pRdmData=%p, nLength=%u", nPort, pRdmData, nLength);
	assert(nPort < config::MAX_PORTS);
	assert(pRdmData != nullptr);
	assert(nLength != 0);

	const auto nUart = _port_to_uart(nPort);

	Dmx::Get()->SetPortDirection(nPort, dmx::PortDirection::OUTP, false);

	for (uint32_t i = 0; i < nLength; i++) {
		while (RESET == usart_flag_get(nUart, USART_FLAG_TBE))
			;
		USART_DATA(nUart) = static_cast<uint16_t>(USART_DATA_DATA & pRdmData[i]);
	}

	while (SET == usart_flag_get(nUart, USART_FLAG_BSY)) {
		static_cast<void>(GET_BITS(USART_DATA(nUart), 0U, 8U));
	}

	udelay(RDM_RESPONDER_DATA_DIRECTION_DELAY);

	Dmx::Get()->SetPortDirection(nPort, dmx::PortDirection::INP, true);
}

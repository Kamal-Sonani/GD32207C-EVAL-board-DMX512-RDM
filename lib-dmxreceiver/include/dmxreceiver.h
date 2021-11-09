/**
 * @file dmxreceiver.h
 *
 */
/* Copyright (C) 2017-2021 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#ifndef DMXRECEIVER_H
#define DMXRECEIVER_H

#include <cstdint>

#include "dmx.h"

#include "lightset.h"

class DMXReceiver: public Dmx {
public:
	DMXReceiver();
	~DMXReceiver();

	void SetOutput(LightSet *pLightSet) {
		m_pLightSet = pLightSet;
	}

	void Start();
	void Stop();

	const uint8_t* Run(int16_t &nLength);

	void Print() {}

private:
	LightSet *m_pLightSet { nullptr };
	bool m_IsActive { false };
	uint8_t m_Data[dmx::buffer::SIZE]; // With DMX Start Code
	uint32_t m_nLength { 0 };
};

#endif /* DMXRECEIVER_H */

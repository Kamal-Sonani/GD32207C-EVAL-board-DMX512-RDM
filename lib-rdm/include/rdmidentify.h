/**
 * @file rdmidentify.h
 *
 */
/* Copyright (C) 2018-2021 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#ifndef RDMIDENTIFY_H_
#define RDMIDENTIFY_H_

#include <cstdint>

#include "ledblink.h"

enum TRdmIdentifyMode {
  IDENTIFY_MODE_QUIET = 0x00,
  IDENTIFY_MODE_LOUD = 0xFF,
} ;

class RDMIdentify {
public:
	RDMIdentify();
	virtual ~RDMIdentify() {}

	virtual void SetMode(TRdmIdentifyMode nMode)=0;

	void On() {
		m_bIsEnabled = true;
		LedBlink::Get()->SetMode(ledblink::Mode::FAST);
	}

	void Off() {
		m_bIsEnabled = false;
		LedBlink::Get()->SetMode(ledblink::Mode::NORMAL);
	}

	bool IsEnabled() const {
		return m_bIsEnabled;
	}

	TRdmIdentifyMode GetMode() {
		return m_nMode;
	}

	static RDMIdentify* Get() {
		return s_pThis;
	}

protected:
	bool m_bIsEnabled { false };
	TRdmIdentifyMode m_nMode { IDENTIFY_MODE_QUIET };

private:
	static RDMIdentify *s_pThis;
};

#endif /* RDMIDENTIFY_H_ */

/**
 * @file hardware.h
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

#ifndef GD32_HARDWARE_H_
#define GD32_HARDWARE_H_

#include <cstdint>
#include <time.h>
#include <uuid/uuid.h>

#include "hwclock.h"

#include "gd32.h"
#include "gd32_adc.h"

class Hardware {
public:
	Hardware();

	uint32_t GetReleaseId() const {
		return 0;	// TODO GetReleaseId
	}

	void GetUuid(uuid_t out);

	uint32_t Millis() {
		extern volatile uint32_t s_nSysTickMillis;
		return s_nSysTickMillis;
	}

	uint32_t GetUpTime() {
		return Millis() / 1000;
	}

	bool SetTime(const struct tm *pTime);
	void GetTime(struct tm *pTime);

	const char *GetBoardName(uint8_t &nLenght) {
		nLenght = 14;
		return "GD32F207C_EVAL";
	}

	const char *GetSysName(uint8_t &nLenght) {
		nLenght = 8;
		return "Embedded";
	}

	uint32_t GetBoardId() {
		return 0;
	}

	const char *GetWebsiteUrl() {
		return "http://gd32-dmx.org";
	}

	void WatchdogInit() {
		const auto status = fwdgt_config(0xFFFF, FWDGT_PSC_DIV16);
		m_bIsWatchdog = (SUCCESS == status);
		if (m_bIsWatchdog) {
			fwdgt_enable();
		}
	}

	void WatchdogFeed() {
		fwdgt_counter_reload();
	}

	void WatchdogStop() {
		m_bIsWatchdog = false;
		fwdgt_config(0xFFFF, FWDGT_PSC_DIV64);
	}

	bool IsWatchdog() const {
		return m_bIsWatchdog;
	}

	bool Reboot();

	void SetRebootHandler(RebootHandler *pRebootHandler) {
		m_pRebootHandler = pRebootHandler;
	}

	float GetCoreTemperature() {
		return gd32_adc_gettemp();
	}

	float GetCoreTemperatureMin() {
		return -40.0f;
	}

	float GetCoreTemperatureMax() {
		return 85.0f;
	}

	static Hardware *Get() {
		return s_pThis;
	}

private:
#if !defined(DISABLE_RTC)
	HwClock m_HwClock;
#endif
	RebootHandler *m_pRebootHandler { nullptr };
	bool m_bIsWatchdog { false };

	static Hardware *s_pThis;
};

#endif /* GD32_HARDWARE_H_ */

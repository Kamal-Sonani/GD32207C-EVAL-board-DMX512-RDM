/**
 * @file rdmpersonality.h
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

#include <cstdint>
#include <cassert>

#ifndef RDMPERSONALITY_H_
#define RDMPERSONALITY_H_

#define RDM_PERSONALITY_DESCRIPTION_MAX_LENGTH		32

class RDMPersonality {
public:
	RDMPersonality(const char *pDescription, uint16_t nSlots);

	uint16_t GetSlots() const {
		return m_nSlots;
	}

	void SetDescription(const char *pDescription) {
		assert(pDescription != nullptr);

		m_nDescriptionLength = 0;

		const auto *pSrc = pDescription;
		auto *pDst = m_aDescription;

		for (uint32_t i = 0; (*pSrc != 0) && (i < RDM_PERSONALITY_DESCRIPTION_MAX_LENGTH); i++) {
			*pDst = *pSrc;
			pSrc++;
			pDst++;
			m_nDescriptionLength++;
		}
	}

	const char *GetDescription() const {
		return m_aDescription;
	}

	uint8_t GetDescriptionLength() const {
		return m_nDescriptionLength;
	}

	void DescriptionCopyTo(char* p, uint8_t &nLength) {
		assert(p != nullptr);

		const auto *pSrc = m_aDescription;
		auto *pDst = p;
		uint8_t i;

		for (i = 0; (i < m_nDescriptionLength) && (i < nLength); i++) {
			*pDst = *pSrc;
			pSrc++;
			pDst++;
		}

		nLength = i;
	}

private:
	uint16_t m_nSlots;
	char m_aDescription[RDM_PERSONALITY_DESCRIPTION_MAX_LENGTH];
	uint8_t m_nDescriptionLength;
};

#endif /* RDMPERSONALITY_H_ */

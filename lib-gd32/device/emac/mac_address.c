/*
 * mac_address.c
 */

#include <stdint.h>

#include "gd32f20x_enet.h"

extern int uart0_printf(const char* fmt, ...);

void mac_address_get(uint8_t paddr[]) {
	const uint32_t mac_lo = *(volatile uint32_t*) (0x1FFFF7EC);
	const uint32_t mac_hi = *(volatile uint32_t*) (0x1FFFF7F0);

	paddr[0] = 2;
	paddr[1] = (mac_lo >> 8) & 0xff;
	paddr[2] = (mac_lo >> 16) & 0xff;
	paddr[3] = (mac_lo >> 24) & 0xff;
	paddr[4] = (mac_hi >> 0) & 0xff;
	paddr[5] = (mac_hi >> 8) & 0xff;

#ifndef NDEBUG
	uart0_printf("%02x:%02x:%02x:%02x:%02x:%02x\n", paddr[0], paddr[1], paddr[2], paddr[3], paddr[4], paddr[5]);
#endif
}

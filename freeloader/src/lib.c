/*
 * lib.c
 *
 * Copyright (C) 2005-2006 The GameCube Linux Team
 * Copyright (C) 2005,2006 Albert Herranz
 * Copyright (C) 2024 leonardus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#include "system.h"

void* xmemcpy(void* dest, const void* src, int count) {
	char *tmp = (char*)dest, *s = (char*)src;

	while (count--)
		*tmp++ = *s++;
	return dest;
}

int xmemcmp(const void* cs, const void* ct, int count) {
	const unsigned char *su1, *su2;
	int                  res = 0;

	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0) break;
	return res;
}

void* xmemset(void* s, int c, int count) {
	char* xs = (char*)s;

	while (count--)
		*xs++ = c;

	return s;
}

static inline void writel(unsigned long b, volatile void* addr) {
	*(volatile unsigned long*)(addr) = b;
}

void rumble(int enable) {
	writel(0x00400000 | ((enable) ? 1 : 0), GCN_SI_C0OUTBUF);
	writel(0x80000000, GCN_SI_SR);
}

void rumble_on(void) {
	rumble(1);
}

void panic(char* text) {
	report("Apploader panic:");
	report(text);
	rumble(1);
	for (;;)
		;
}

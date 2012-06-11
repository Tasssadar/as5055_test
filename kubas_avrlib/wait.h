/*
Cekani
Funguje od 2 do 4294967294 us
Zadava se v us pro F_CPU 16000000
*/

#ifndef KUBAS_WAIT_H
#define KUBAS_WAIT_H

void wait(uint32_t time)
{
	time -= 2;
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	for(uint32_t i = 0; i < time; ++i)
	{
		for(uint8_t j = 0; j < 5; ++j)
		{
			__asm__ volatile ("nop");
		}
	}
}

#endif
#ifndef KUBAS_EEPROM
#define KUBAS_EEPROM

#include <avr/io.h>
#include <avr/interrupt.h>

namespace kubas
{

class eeprom_t {

#if defined (__AVR_ATmega48__)
	static const uint16_t max_add = 255;
#elif (defined (__AVR_ATmega88__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega16__))
	static const uint16_t max_add = 511;
#elif defined (__AVR_ATmega32__)
	static const uint16_t max_add = 1023;
#elif (defined (__AVR_ATmega128__) || defined (__AVR_ATmega1284P__))
	static const uint16_t max_add = 4095;
#else
	#error "Please define EEPROM_SIZE"
	static const uint16_t max_add = EEPROM_SIZE;
#endif

#if (defined (_AVR_IOMX8_H_) || defined (__AVR_ATmega1284P__))
	#define eemwe EEMPE
	#define eewe  EEPE
#else
	#define eemwe EEMWE
	#define eewe  EEWE
#endif
	
public:

	uint8_t read_byte(uint16_t address)
	{
		if(address <= max_add)
		{
			EEAR = address;
			EECR |= (1<<EERE);
			return EEDR;
		}
		return 0xFF;
	}

	void write_byte(uint16_t address, uint8_t data)
	{
		if(address <= max_add)
		{
			cli();
			EEAR = address;
			EEDR = data;
			EECR = (1<<eemwe);
			EECR |= (1<<eewe);
			while((EECR & (1<<eewe)) != 0);
			{
			}
			sei();
		}
	}
	
	uint16_t write_block(uint16_t address, uint16_t length, uint8_t const * data)
	{
		uint16_t i = 0;
		for(; i != length; ++i)
			write_byte(address + i, *(data + i));
		return address + i;
	}

	uint16_t read_block(uint16_t address, uint16_t length, uint8_t *data)
	{
		uint16_t i = 0;
		for(; i != length; ++i)
			*(data + i) = read_byte(address + i);
		return address + i;
	}

	template <typename T>
	uint16_t write(uint16_t address, T data)
	{
		write_block(address, sizeof data, (uint8_t const *)&data);
		return address + (sizeof data);
	}

	template <typename T>
	uint16_t read(uint16_t address, T &data)
	{
		T result;
		read_block(address, sizeof result, (uint8_t *)&result);
		data = result;
		return address + (sizeof data);
	}

};

#ifdef KUBAS_EXTERN_OBJECTS
extern eeprom_t eeprom;
#else
eeprom_t eeprom;
#endif

} // end of namespace kubas

#endif
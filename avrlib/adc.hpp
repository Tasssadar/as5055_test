#ifndef AVRLIB_ADC_HPP
#define AVRLIB_ADC_HPP

#include <avr/io.h>

namespace avrlib {
	
enum adc_reference {aref, avcc, reserved, int1v1};
enum adc_clock_prescaller {f_cpu_2, f_cpu_4, f_cpu_8, f_cpu_16, f_cpu_32, f_cpu_64, f_cpu_128};
enum adc_aligment {right, left};

class sync_adc
{
public:
	explicit sync_adc(uint8_t channel, bool reverse = false, adc_aligment aligment = right, adc_reference reference = aref, adc_clock_prescaller clock = f_cpu_128)
		: m_channel(channel), m_reverse(reverse), m_aligment(aligment), m_reference(reference<<6), m_clock(clock+1)
	{
		ADCSRA |= (1<<ADEN) | m_clock;
		ADMUX = (m_aligment<<ADLAR) | m_reference | m_channel;
	}

	void start()
	{
		ADCSRA |= (1<<ADSC);
	}

	bool running() const
	{
		return (ADCSRA & (1<<ADSC)) != 0;
	}

	uint16_t value() const
	{
		uint16_t res = ADCL;
		res |= ADCH << 8;
		return res;
	}

	uint16_t operator()()
	{
		ADMUX = (m_aligment<<ADLAR) | m_reference | m_channel;
		ADCSRA |= (1<<ADSC);
		while ((ADCSRA & (1<<ADIF)) == 0)
		{
		}
		/*ADCSRA |= (1<<ADSC);
		while ((ADCSRA & (1<<ADIF)) == 0)
		{
		}*/

		uint16_t res = ADCL;
		res |= ADCH << 8;
		
		ADCSRA |= (1<<ADIF);

		if (m_reverse)
			res = -res;

		return res;
	}

private:
	uint8_t m_channel;
	bool m_reverse;
	bool m_aligment;
	uint8_t m_reference;
	uint8_t m_clock;
};


class async_adc
{
public:
	explicit async_adc(uint8_t channel, bool reverse = false)
		: m_channel(channel), m_reverse(reverse), m_value(0)
	{
	}

	void start()
	{
		ADMUX = (1<<ADLAR) | m_channel;
		ADCSRA |= (1<<ADSC);
	}

	bool process()
	{
		if ((ADCSRA & (1<<ADSC)) != 0)
			return false;

		m_value = ADCL;
		m_value |= ADCH << 8;

		if (m_reverse)
			m_value = -m_value;

		return true;
	}

	uint16_t value() const
	{
		return m_value;
	}

private:
	uint8_t m_channel;
	bool m_reverse;
	uint16_t m_value;
};

}

#endif

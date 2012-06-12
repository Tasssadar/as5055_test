/*
 * as5055_test.c
 *
 * Created: 6.4.2012 14:12:27
 *  Author: Kubas
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <util/delay.h>

#include "avrlib/porta.hpp"
#include "avrlib/portb.hpp"
#include "avrlib/portc.hpp"
#include "avrlib/portd.hpp"
#include "avrlib/porte.hpp"
#include "avrlib/portf.hpp"
#include "avrlib/portg.hpp"
#include "avrlib/pin.hpp"

#include "kubas_avrlib/uart.h"
#include "kubas_avrlib/format.h"
#include "kubas_avrlib/util.h"

using namespace kubas;
using namespace avrlib;

iostream_t <uart_t> debug(uart);

typedef pin<portg, 0> en0_cs;
typedef pin<portg, 1> en1_cs;

typedef pin<portb, 3> MISO;
typedef pin<portb, 2> MOSI;
typedef pin<portb, 1> SCK;
typedef pin<portb, 0> SS;

class encoder_t
{
public:
	
	typedef uint8_t state_t;
	typedef int16_t value_t;
	typedef int32_t distance_t;
	typedef int16_t speed_t;
	typedef int16_t acceleration_t;
	
	encoder_t()
		:m_state(0), m_value(0), m_value_last(0), m_distance(0), m_distance_last(0),
		 m_speed(0), m_speed_last(0), m_acceleration(0), m_agc(0), m_speed_timer(8)
	{
			
	}
	
	inline state_t state() const { return m_state; }
	inline value_t value() const { return m_value; }
	inline distance_t distance() const { return m_distance; }
	inline speed_t speed() const { return m_speed; }
	inline acceleration_t acceleration() const { return m_acceleration; }
	inline uint8_t agc() const { return m_agc; }
	
	void clear()
	{
		m_state = 0;
		m_value_last = m_value;
		m_distance = 0;
		m_distance_last = 0;
		m_speed = 0;
		m_speed_last = 0;
		m_acceleration = 0;
	}
	
	void compute()
	{
		value_t delta = m_value - m_value_last;
		if(abs(delta) > 2048)
		{
			if (delta < 0)
				delta = 4095 - m_value_last + m_value;
			else
				delta = 4095 - m_value + m_value_last;
		}
		m_value_last = m_value;
		if(abs(delta) < 4)
			delta = 0;
		m_distance += delta;
		m_speed = m_distance - m_distance_last;
		m_distance_last = m_distance;
		m_acceleration = m_speed - m_speed_last;
		m_speed_last = m_speed;
	}
	
//static functions
	
	static void init()
	{
		en0_cs::make_high();
		en1_cs::make_low();
		MISO::set(true);//pull-up
		MOSI::output(true);
		SCK::output(true);
		SS::set(true);//pull-up
		SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPR0) | (1<<CPHA);
	}
	
	static void start(const uint16_t &reg = c_angular_data_register)
	{
		m_spi_cmd = reg;
		en0_cs::clear();
		SPDR = m_spi_cmd >> 8;
	}
	
	static inline bool read() { return m_read; }
		
	static inline void processed() { m_read = false; }
	
	static void process();

private:

	void set(value_t v)
	{
		switch (m_spi_last_cmd)
		{
		case c_angular_data_register:
			set_ang(v);
			break;
		case c_automatic_gain_control_register:
			set_agc(v);
			break;
		case c_clear_error_flag_register:
			break;
		}
	}
	
	void set_ang(value_t &v)
	{
		m_state = v & 2;
		v >>= 2;
		m_value = v & ((1<<12)-1);
		m_state |= (v & ((1<<12)|(1<<13)))>>10;
	}
	
	void set_agc(value_t &v)
	{
		m_state |= v & 2;
		v >>= 2;
		m_agc = v & ((1<<6)-1);
	}
	
	volatile state_t m_state;
	volatile value_t m_value;
	volatile value_t m_value_last;
	volatile distance_t m_distance;
	volatile distance_t m_distance_last;
	volatile speed_t m_speed;
	volatile speed_t m_speed_last;
	volatile acceleration_t m_acceleration;
	volatile uint8_t m_agc;
	uint8_t m_speed_timer;

	static uint8_t m_temp;
	static volatile uint8_t m_spi_state;
	static volatile uint16_t m_spi_cmd;
	static volatile uint16_t m_spi_last_cmd;
	static volatile bool m_read;
	
public:
	
	static const uint16_t c_automatic_gain_control_register	= ((1<<15)|(0x3FF8<<1)|1)^1;
	static const uint16_t c_angular_data_register			= ((1<<15)|(0x3FFF<<1)|0)^1;
	static const uint16_t c_clear_error_flag_register		= ((1<<15)|(0x3380<<1)|1)^1;
	
	static const uint8_t 
};

encoder_t en_left;
encoder_t en_right;

uint8_t encoder_t::m_temp = 0;
volatile uint8_t encoder_t::m_spi_state = 0;
volatile uint16_t encoder_t::m_spi_cmd = encoder_t::c_angular_data_register;
volatile uint16_t encoder_t::m_spi_last_cmd = encoder_t::c_angular_data_register;
volatile bool encoder_t::m_read = 0;

void encoder_t::process()
{
	switch (m_spi_state++)
	{
	case 0:
	case 2:
		m_temp = SPDR;
		SPDR = m_spi_cmd & 0xFF;
		break;
			
	case 1:
		en0_cs::set();
		en_left.set((m_temp<<8)|SPDR);
		en1_cs::clear();
		SPDR = m_spi_cmd >> 8;
		break;
			
	case 3:
		en1_cs::set();
		en_right.set((m_temp<<8)|SPDR);
		m_spi_state = 0;
		m_spi_last_cmd = m_spi_cmd;
		if(m_spi_cmd == c_angular_data_register)
		{
			start(c_automatic_gain_control_register);
			break;
		}
		m_read = true;
		break;
	}
}

ISR(SPI_STC_vect)
{
	encoder_t::process();
}

volatile bool clear_error_flag = false;

ISR(TIMER2_COMP_vect)
{
	if(clear_error_flag)
	{
		encoder_t::start(encoder_t::c_clear_error_flag_register);
		clear_error_flag = false;
	}		
	else
		encoder_t::start();
}

#define PWM_RESOLUTION 50

#include "drivers.h"

typedef pin<porte, 6> in_la;
typedef pin<porte, 2> in_lb;
typedef pin<porte, 3> en_la;
typedef pin<portf, 1> en_lb;
typedef pin<portg, 4> in_ra;
typedef pin<porte, 7> in_rb;
typedef pin<portg, 3> en_ra;
typedef pin<portb, 4> en_rb;

typedef pin<porte, 4> stop_button;

driver_t <in_la, in_lb, en_la, en_lb, ocr1a> left;
driver_t <in_rb, in_ra, en_ra, en_rb, ocr1b> right;

volatile uint16_t stop_button_filter_timer = 2000;

int main(void)
{
	in_la::output(true);
	in_lb::output(true);
	in_ra::output(true);
	in_rb::output(true);
	
	DDRB |= (1<<6)|(1<<5);//PWMs
	
	encoder_t::init();
	
	uart.init(38400);
	_delay_ms(1);
	sei();
	
	encoder_t::start();
	while(!encoder_t::read()) {}
	en_left.compute();
	en_right.compute();
	en_left.clear();
	en_right.clear();
	encoder_t::processed();
	
	OCR2 = 80;//5.12ms
	TIMSK |= (1<<OCIE2);
	TCCR2 = (1<<WGM21)|5;
	
	OCR1BH = 0;
	OCR1BL = 0;
	OCR1AH = 0;
	OCR1AL = 0;
	ICR1   = PWM_RESOLUTION;
	TCCR1A = (1<<COM1B1)|(1<<COM1A1)|(1<<WGM11);
	TCCR1B = (1<<CS11)|(1<<WGM13);
	
	debug<<endl<<endl<<" OK "<<endl;
	debug.align(8);
	
	const string space("; ____ ");
	char ch;
	
	int16_t power = 0;
	
	DDRB |= (1<<7);
	
	for(;;)
	{
		if (encoder_t::read())
		{
			PORTB |= (1<<7);
			en_left.compute();
			en_right.compute();
			encoder_t::processed();
			debug<<"lp:"<<en_left.value()<<space
				 <<"ld:"<<en_left.distance()<<space
				 <<"lv:"<<en_left.speed()<<space
				 <<"la:"<<en_left.acceleration()<<space
				 <<"lg;"<<en_left.agc()<<space
				 <<"ls:"<<en_left.state()<<space
				 <<"    ||||     "
				 <<"rp:"<<en_right.value()<<space
				 <<"rd:"<<en_right.distance()<<space
				 <<"rv:"<<en_right.speed()<<space
				 <<"ra:"<<en_right.acceleration()<<space
				 <<"rg;"<<en_right.agc()<<space
				 <<"rs:"<<en_right.state()<<space
				 <<endl;//<<endl;
			//uart.wait();
			PORTB &= ~(1<<7);
		}
		if(debug.peek(ch))
		{
			switch (ch)
			{
			case '\r':
				debug<<endl;
				break;
				
			case 'C':
				en_left.clear();
				en_right.clear();
				break;
				
			case 'c':
				debug<<"clear"<<"...";
				clear_error_flag = true;
				while(clear_error_flag) {}
				debug<<"started"<<"...";
				while(!encoder_t::read()) {}
				debug<<"done."<<endl;
				break;
				
			case ' ':
			case '*':
				power = 0;
				right.power(power);
				debug<<"power: "<<power<<" (stop)"<<endl;
				break;
				
			case '+':
				if(power <= PWM_RESOLUTION)
					++power;
				right.power(power);
				debug<<"power: "<<power<<endl;
				uart.rx_clear();
				break;
				
			case '-':
				if(power >= -PWM_RESOLUTION)
					--power;
				right.power(power);
				debug<<"power: "<<power<<endl;
				uart.rx_clear();
				break;
				
			case 'M':
				uart.init(2000000);
				uart.get();
				break;
				
			case 'm':
				uart.init(38400);
				uart.get();
				break;
			}
		}
	}
}
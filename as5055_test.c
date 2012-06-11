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
//#include "avrlib/porte.hpp"
//#include "avrlib/portf.hpp"
//#include "avrlib/portg.hpp"
#include "avrlib/pin.hpp"

#include "kubas_avrlib/uart.h"
#include "kubas_avrlib/format.h"
#include "kubas_avrlib/util.h"

using namespace kubas;
using namespace avrlib;

iostream_t <uart_t> debug(uart);

typedef pin<portb, 3> en0_cs;
//typedef pin<portb, 3> en1_cs;

typedef pin<portb, 6> MISO;
typedef pin<portb, 5> MOSI;
typedef pin<portb, 7> SCK;
typedef pin<portb, 4> SS;

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
		en0_cs::make_low();
//		en1_cs::make_low();
		MISO::set(true);//pull-up
		MOSI::output(true);
		SCK::output(true);
		SS::set(true);//pull-up
		SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPR0) | (1<<CPHA);
                //SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<CPHA);
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
	
//	static const uint8_t 
};

encoder_t en_left;
//encoder_t en_right;

uint8_t encoder_t::m_temp = 0;
volatile uint8_t encoder_t::m_spi_state = 0;
volatile uint16_t encoder_t::m_spi_cmd = encoder_t::c_angular_data_register;
volatile uint16_t encoder_t::m_spi_last_cmd = encoder_t::c_angular_data_register;
volatile bool encoder_t::m_read = 0;

/*
void encoder_t::process()
{
	switch (m_spi_state++)
	{
	case 0:
	case 2:
		m_temp = SPDR;
		SPDR = m_spi_cmd & 0xFF;
                debug<<endl<<endl<<" OK a " << m_spi_cmd << " " << m_spi_state <<endl;
		break;
			
	case 1:
                debug<<endl<<endl<<" OK b"<<endl;
		en0_cs::set();
		en_left.set((m_temp<<8)|SPDR);
		//en1_cs::clear();
		SPDR = m_spi_cmd >> 8;
		break;
			
	case 3:
                debug<<endl<<endl<<" OK c"<<endl;
		//en1_cs::set();
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
}*/

void encoder_t::process()
{
        switch (m_spi_state++)
        {
        case 0:
                m_temp = SPDR;
                SPDR = m_spi_cmd & 0xFF;
                break;
                        
        case 1:
                en0_cs::set();
                en_left.set((m_temp<<8)|SPDR);
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
/*
//#include "drivers.h"

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
driver_t <in_rb, in_ra, en_ra, en_rb, ocr1b> right;*/

volatile uint16_t stop_button_filter_timer = 2000;


void write16(uint16_t val)
{
        uart.write(val >> 8);
        uart.write(val & 0xFF);
}

void write32(uint32_t val)
{
        uart.write(val >> 24);
        uart.write(val >> 16);
        uart.write(val >> 8);
        uart.write(val & 0xFF);
}

class Packet
{
public:
    Packet()
    {
            clear();
    }
    Packet(uint8_t cmd)
    {
            clear();
            m_cmd = cmd;
    }

    void setCmd(uint8_t cmd)
    {
        m_cmd = cmd;
    }

    void clear()
    {
            m_cmd = 0;
            m_len = 0;
            m_ritr = 0;
            m_recv = 0;
    }
    bool add(uint8_t ch)
    {     
        switch(m_recv)
        {
                case 0:
                    if(ch != 0xFF)
                        return false;
                    break;
                case 1:
                    if(ch != 0x00 && ch != 0x01)
                        return false;
                    break;
                case 2:
                    m_len = ch;
                    break;
                case 3:
                    m_cmd = ch;
                    break;
                default:
                {
                    if(m_recv-3 >= m_len)
                        return false;

                    m_data[m_recv - 4] = ch;
                    break;
                }
        }
        ++m_recv;
        return true;
    }
    bool isValid()
    {
        return (m_len && 3+m_len == m_recv);
    }
    void send()
    {
        uart.write(0xFF);
        uart.write(0x00);
        uart.write(m_len+1);
        uart.write(m_cmd);

        for(uint8_t i = 0; i < m_len; ++i)
            uart.write(m_data[i]);
    }

    uint8_t getCmd() const { return m_cmd; }
    
    void setWriteItr(int pos)
    {
        m_len = pos;
    }

    void write8(const uint8_t& v)
    {
        m_data[m_len++] = v;
    }
    void write16(const int16_t& v)
    {
        m_data[m_len++] = (v >> 8);
        m_data[m_len++] = (v & 0xFF);
    }
    void write32(const uint32_t& v)
    {
        m_data[m_len++] = (v >> 24);
        m_data[m_len++] = (v >> 16);
        m_data[m_len++] = (v >> 8);
        m_data[m_len++] = (v & 0xFF);
    }

    uint8_t read8()
    {
         return m_data[m_ritr++];
    }
    uint16_t read16()
    {
        uint16_t res = (m_data[m_ritr++] << 8);
        res |= m_data[m_ritr++];
        return res;
    }
    
    uint32_t read32()
    {
         uint32_t res = ((uint32_t)m_data[m_ritr++] << 24);
        res |= ((uint32_t)m_data[m_ritr++] << 16);
        res |= (m_data[m_ritr++] << 8);
        res |= m_data[m_ritr++];
        return res;
    }

private:
    uint8_t m_len;
    uint8_t m_cmd;
    uint8_t m_data[20];

    uint8_t m_recv;
    uint8_t m_ritr;
};

int main(void)
{
/*
	in_la::output(true);
	in_lb::output(true);
	in_ra::output(true);
	in_rb::output(true);
	
	DDRB |= (1<<6)|(1<<5);//PWMs
*/
	encoder_t::init();
	
	uart.init(38400);
	_delay_ms(1);
	sei();
        encoder_t::start();
        debug<<endl<<endl<<" OK 2"<<endl;
	
        debug<<endl<<endl<<" OK 3"<<endl;
	while(!encoder_t::read()) {}
	debug<<endl<<endl<<" OK 4"<<endl;
	en_left.compute();
        debug<<endl<<endl<<" OK 5"<<endl;
//	en_right.compute();
	en_left.clear();
        debug<<endl<<endl<<" OK 6"<<endl;
//	en_right.clear();
	encoder_t::processed();
        debug<<endl<<endl<<" OK 7"<<endl;
	
	OCR2 = 80;//5.12ms
	TIMSK |= (1<<OCIE2);
	TCCR2 = (1<<WGM21)|5;
	/*
	OCR1BH = 0;
	OCR1BL = 0;
	OCR1AH = 0;
	OCR1AL = 0;
	ICR1   = PWM_RESOLUTION;
	TCCR1A = (1<<COM1B1)|(1<<COM1A1)|(1<<WGM11);
	TCCR1B = (1<<CS11)|(1<<WGM13);*/
	
	debug<<endl<<endl<<" OK "<<endl;
	debug.align(8);
	
	const string space("; ____ ");
	char ch;
	
	int16_t power = 0;
	
	//DDRB |= (1<<7);
	Packet pkt;
    Packet sendPkt;
	for(;;)
	{
                if(encoder_t::read())
                {
                        en_left.compute();
                        
                        encoder_t::processed();
                        
                        sendPkt.setWriteItr(0);
                        sendPkt.write16(en_left.value());         // 0
                        sendPkt.write32(en_left.distance());      // 2
                        sendPkt.write16(en_left.speed());         // 6
                        sendPkt.write16(en_left.acceleration());  // 8
                        sendPkt.write8(en_left.agc());        // 10
                        sendPkt.write8(en_left.state());      // 11
                        sendPkt.send();
                }
                
                if(debug.peek(ch))
                {
                    pkt.add(ch);
                    if(!pkt.isValid())
                        continue;
                    switch(pkt.getCmd())
                    {
                        case 0:
                            en_left.clear();
                            break;
                        case 1:
                            clear_error_flag = true;
                            while(clear_error_flag) {}
                            while(!encoder_t::read()) {}
                            break;
                    }
                    pkt.clear();
                }
                     
/*                if (encoder_t::read())
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
		}*/
	}
}
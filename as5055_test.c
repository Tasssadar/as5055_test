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
#include "avrlib/pin.hpp"

#include "kubas_avrlib/uart.h"
#include "kubas_avrlib/format.h"
#include "kubas_avrlib/util.h"

using namespace kubas;
using namespace avrlib;

iostream_t <uart_t> debug(uart);

#include "packet.h"

typedef pin<portb, 3> en0_cs;

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
    
    static const uint16_t c_automatic_gain_control_register    = ((1<<15)|(0x3FF8<<1)|1)^1;
    static const uint16_t c_angular_data_register            = ((1<<15)|(0x3FFF<<1)|0)^1;
    static const uint16_t c_clear_error_flag_register        = ((1<<15)|(0x3380<<1)|1)^1;
};

encoder_t en_left;

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

int main(void)
{
    encoder_t::init();
    
    uart.init(38400);
    _delay_ms(1);
    sei();
    
    encoder_t::start();

    while(!encoder_t::read()) {}

    en_left.compute();
    en_left.clear();
    encoder_t::processed();
    
    OCR2 = 80;//5.12ms
    TIMSK |= (1<<OCIE2);
    TCCR2 = (1<<WGM21)|5;

    char ch;    
    Packet pkt;
    Packet sendPkt;
    bool send = true;

    for(;;)
    {
        if(encoder_t::read())
        {
            en_left.compute();
            
            encoder_t::processed();
            
            if(send)
            {
                sendPkt.setWriteItr(0);
                sendPkt.write16(en_left.value());         // 0
                sendPkt.write32(en_left.distance());      // 2
                sendPkt.write16(en_left.speed());         // 6
                sendPkt.write16(en_left.acceleration());  // 8
                sendPkt.write8(en_left.agc());        // 10
                sendPkt.write8(en_left.state());      // 11
                sendPkt.send();
            }
        }
        
        if(debug.peek(ch))
        {
            if(!pkt.add(ch))
            {
                pkt.clear();
                continue;
            }

            if(!pkt.isValid())
                continue;

            switch(pkt.getCmd())
            {
                case 0: // clear distance
                    en_left.clear();
                    break;
                case 1: // clear error flag
                    clear_error_flag = true;
                    while(clear_error_flag) {}
                    while(!encoder_t::read()) {}
                    break;
                case 2: // toggle sending
                    send = pkt.read8();
                    break;
            }
            pkt.clear();
        }
    }
}

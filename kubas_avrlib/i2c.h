#ifndef __KUBAS_I2C_H_
#define __KUBAS_I2C_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#include "queue.h"
#include "pair.h"

#ifndef I2C_TIMEOUT
#define I2C_TIMEOUT 1000
#endif

#ifndef I2C_TX_BUFF
#define I2C_TX_BUFF 5
#endif

namespace kubas {

class i2c_t
{
	typedef pair<uint8_t, bool> tx_buff_type;
	queue<tx_buff_type, I2C_TX_BUFF> tx_buff;
	uint8_t rlength;
	uint8_t * rx_buff;
	volatile bool _busy;
	volatile bool _succes;
	volatile bool _ready;
	volatile bool i2c_timeout_timer_run;
	volatile uint16_t i2c_timeout_timer;

	inline void _start()
	{
		i2c_timeout_timer_run = true;
		i2c_timeout_timer = I2C_TIMEOUT;
		_busy = true;
		TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE) | (1<<TWSTA);
	}
	inline void _next(bool ack = true)
	{
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE) | (ack<<TWEA);
	}
	inline void _stop()
	{
		_busy = false;
		i2c_timeout_timer_run = false;
		TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE) | (1<<TWSTO);
	}

public:

	i2c_t()
	{
		_busy = false;
		_succes = false;
		_ready = false;
		i2c_timeout_timer_run = false;
	}

	void init ()
	{
		TWBR = 12;//SCL freq 400kHz @ 16HMz without prescaler
		TWSR = 0;//without prescaler
		TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
		_busy = false;
		_ready = true;
	}
	void stop()
	{
		_stop();
		TWCR = 0;
		_busy = false;
		_succes = false;
		_ready = false;
		i2c_timeout_timer_run = false;
	}

	bool ready () { return _ready; }

	bool busy () { return _busy; }
		
	void wait() { while(busy()) {}; }
	
	bool succes ()
	{
		if(!_busy)
			return _succes;
		return false;
	}

	void write (uint8_t addr, uint8_t * data, uint8_t length)
	{
		if(!_ready)
		{
			_succes = false;
			return;
		}
		//while(_busy) {}
		_succes = true;
		//tx_buff.clear();
		tx_buff.push(make_pair(addr<<1, true));
		for(uint8_t i = 0; i != length; ++i)
			tx_buff.push(make_pair(*(data+i), false));
		_start();
	}

	void read (uint8_t addr, uint8_t * data, uint8_t length)
	{
		if(length == 0 || !_ready)// length < 1
		{
			_succes = false;
			return;
		}
		//while(_busy) {}
		rlength = length;
		rx_buff = data;
		_succes = true;
		//tx_buff.clear();
		tx_buff.push(make_pair((addr<<1)+1, true));
		_start();
	}
	
	void clear()
	{
		_stop();
		tx_buff.clear();
	}

	void procces (uint8_t state)
	{
// 		uint8_t base = bus.base();
// 		bus.base(16);
// 		bus<<"->"<<state<<endl;
// 		bus.base(base);
		switch(state)
		{
			case 0x08://start transmitted
			case 0x10://repeated start transmitted
				TWDR = tx_buff.pop().first;
				_next();
				break;

			case 0x18://SLA+W transmitted, ACK returned
			case 0x28://data  transmitted, ACK returned
				if(tx_buff.empty())
					_stop();
				else
				{
					if(tx_buff.front().second)
						_start();
					else
					{
						TWDR = tx_buff.pop().first;
						_next();
					}
				}
				break;

			case 0x20://SLA+W transmitted, NACK returned
			case 0x30://data  transmitted, NACK returned
			case 0x48://SLA+R transmitted, NACK returned
			case 0x38://arbitration lost in SLA+R/W or data (transmitter) or NACK (receiver)
				_succes = false;
				_stop();
				break;

			case 0x40://SLA+R transmitted, ACK returned
				_next(rlength != 1);
				break;

			case 0x50://data received, ACK returned
				*(rx_buff++) = TWDR;
				_next(--rlength != 1);
				break;

			case 0x58://data received, NACK returned
				*(rx_buff) = TWDR;
				if(tx_buff.empty())
					_stop();
				else
					_start();
				break;

			case 0x00://bus error (illegal START or STOP condition)
				_succes = false;
				_stop();

			default:
				break;
		}
	}

	inline void timer_process()
	{
		if(i2c_timeout_timer_run)
			if(--i2c_timeout_timer == 0)
			{
				_succes = false;
				_stop();
				i2c_timeout_timer_run = false;
				//err_led::set();
				//err_led_time = 500;
				//debug<<"error"<<endl;
				//_error |= DECODER_ERROR_TIMEOUT;
			}
	}

}; i2c_t i2c;

}// end of namespace kubas

ISR(TWI_vect)
{
	kubas::i2c.procces(TWSR & 0xF8);
}

#endif
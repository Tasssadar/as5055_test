#ifndef KUBAS_UART
#define KUBAS_UART

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "queue.h"

namespace kubas
{

#ifndef UART_RX_BUF
	#if defined (__AVR_ATmega48__)
		#define UART_RX_BUF 32
	#else
		#define UART_RX_BUF 96
	#endif
#endif

#ifdef UART_ASYNC_TX
	#ifndef UART_TX_BUF
		#if defined (__AVR_ATmega48__)
			#define UART_TX_BUF 32
		#else
			#define UART_TX_BUF 96
		#endif
	#endif
#endif

#if ((defined (__AVR_ATmega128__))||(defined (_AVR_IOM1284P_H_)))
	#ifndef UART1_RX_BUF
		#define UART1_RX_BUF 96
	#endif
	#ifdef UART1_ASYNC_TX
		#ifndef UART1_TX_BUF
			#define UART1_TX_BUF 96
		#endif
	#endif
	#ifndef DONT_USE_BOOTLOADER
		#ifndef BOOTLOADER_PORT
			#define BOOTLOADER_PORT 0
		#endif
	#endif
#endif

#ifndef endl
	#define endl "\n\r"
#endif

#ifndef UCSRA
	#define UCSRA UCSR0A
#endif
#ifndef UCSRB
	#define UCSRB UCSR0B
#endif
#ifndef UCSRC
	#define UCSRC UCSR0C
#endif
#ifndef UBRRH
	#define UBRRH UBRR0H
#endif
#ifndef UBRRL
	#define UBRRL UBRR0L
#endif
#ifndef UDR
	#define UDR UDR0
#endif
#ifndef U2X
	#define U2X U2X0
#endif
#ifndef RXCIE
	#define RXCIE RXCIE0
#endif
#ifndef RXEN
	#define RXEN RXEN0
#endif
#ifndef TXEN
	#define TXEN TXEN0
#endif
#ifndef URSEL
	#define URSEL 0
#endif
#ifndef UCSZ0
	#define UCSZ0 UCSZ00
#endif
#ifndef UCSZ1
	#define UCSZ1 UCSZ01
#endif
#ifndef TXCIE
	#define TXCIE TXCIE0
#endif
#ifndef TXC
	#define TXC TXC0
#endif
#ifndef UDRIE
	#define UDRIE UDRIE0
#endif
#ifndef FE
	#define FE FE0
#endif
#ifndef USART0_TX_vect
	#define USART0_TX_vect USART_TX_vect
#endif
#ifndef USART0_RX_vect
	#define USART0_RX_vect USART_RX_vect
#endif
#ifndef USART_UDRE_vect
	#define USART_UDRE_vect USART0_UDRE_vect
#endif
#ifndef USART_TXC_vect
	#define USART_TXC_vect USART0_TX_vect
#endif
#ifndef USART_RXC_vect
	#define USART_RXC_vect USART0_RX_vect
#endif

#ifndef BOOTLOADER_INIT_SEQ
	#define BOOTLOADER_INIT_SEQ { 0x74, 0x7E, 0x7A, 0x33 }
#endif

#ifndef DONT_USE_UART

class uart_t {

#ifndef UART_EXTERN_RX_INT
	queue <char, UART_RX_BUF> m_rx;
#endif
#ifdef UART_ASYNC_TX
	queue <char, UART_TX_BUF> m_tx;
#endif

public:

#ifndef UART_EXTERN_RX_INT
	void data_in(char ch)
	{
		m_rx.push(ch);
	}
#endif

#ifdef UART_ASYNC_TX
	bool data_out(char &ch)
	{
		if(m_tx.empty())
			return false;
		ch = m_tx.pop();
		return true;
	}

	inline bool is_send()
	{
		return m_tx.empty();
	}
	
	void wait()
	{
		while(!m_tx.empty()) {}
	}
	
	void tx_clear()
	{
		m_tx.clear();
		UCSRB &= ~(1<<UDRIE);
#ifdef RS485_CTRL_PIN
		tx_end_process();
#endif
	}
#endif //UART_ASYNC_TX
	
	inline void rx_clear()
	{
		m_rx.clear();
	}
	
	inline void clear()
	{
#ifdef UART_ASYNC_TX
		tx_clear();
#endif
		rx_clear();
	}

	uint32_t init(uint32_t speed)
	{
		UCSRA = (1<<U2X);
		UCSRB = ((1<<RXCIE)|(1<<RXEN)|(1<<TXEN));
		UCSRC = ((1<<URSEL)|(1<<UCSZ0)|(1<<UCSZ1));
		speed = uint16_t(F_CPU / (8 * speed) - 1);
		UBRRH = ((speed&0xFF00)>>8);
		UBRRL = (speed&0x00FF);
#ifdef RS485_CTRL_PIN
		RS485_CTRL_PIN::output(true);
		RS485_CTRL_PIN::clear();
#ifdef UART_ASYNC_TX
		UCSRB |= (1<<TXCIE);
#endif
#endif
		return F_CPU/(float)(8*(((UBRRH<<8)|UBRRL)+1));
	}
	void write(const char &data)
	{
#ifdef UART_ASYNC_TX
		while(!m_tx.push(data)) {}
#ifdef RS485_CTRL_PIN
		RS485_CTRL_PIN::set();
#endif
		UCSRB |= (1<<UDRIE);
#else //UART_ASYNC_TX
		write_immediately(data);
#endif //UART_ASYNC_TX
	}
	
	void write_immediately(const char &data)
	{
#ifdef RS485_CTRL_PIN
		RS485_CTRL_PIN::set();
#endif
		UCSRA |= (1<<TXC);
		UDR = data;
		while((UCSRA & (1<<TXC)) == 0){}
#ifdef RS485_CTRL_PIN
		RS485_CTRL_PIN::clear();
#endif
	}
	

#ifndef UART_EXTERN_RX_INT
	bool peek(char & data)
	{
		if(m_rx.empty())
			return false;
		data = m_rx.pop();
		return true;
	}

	char get()
	{
		char data;
		while(!peek(data)) {}
		return data;
	}
#endif

	inline void rx_process()
	{
		char data = UDR;
		if((UCSRA & (1<<FE)) == (1<<FE))
			return;
		data_in(data);
#ifndef DONT_USE_BOOTLOADER
		static const unsigned char bootSeq[] = BOOTLOADER_INIT_SEQ;
		static uint8_t state = 0;
		if (data == bootSeq[state])
		{
			if (++state == 4)
				wdt_enable(WDTO_15MS);
		}
		else
			state = 0;
#endif //DONT_USE_BOOTLOADER
	}
	
#ifdef UART_ASYNC_TX
	inline void tx_process()
	{
		char data;
		if(data_out(data))
			UDR = data;
		else
			UCSRB &= ~(1<<UDRIE);
	}
	
#ifdef RS485_CTRL_PIN
	inline void tx_end_process()
	{
		RS485_CTRL_PIN::clear();
	}
#endif
#endif //UART_ASYNC_TX

}; 

#ifdef KUBAS_EXTERN_OBJECTS
extern uart_t uart;
#else //KUBAS_EXTERN_OBJECTS
uart_t uart;

#ifndef UART_EXTERN_RX_INT
ISR(USART_RXC_vect)
{
	uart.rx_process();
}
#endif //UART_EXTERN_RX_INT

#ifdef UART_ASYNC_TX
ISR(USART_UDRE_vect)
{
	uart.tx_process();
}

#ifdef RS485_CTRL_PIN
ISR(USART_TXC_vect)
{
	uart.tx_end_process();
}
#endif //RS485_CTRL_PIN
#endif //UART_ASYNC_TX
#endif //KUBAS_EXTERN_OBJECTS
#endif //DONT_USE_UART


//*********************************** USART1 **********************************
#if ((defined (__AVR_ATmega128__))||(defined (_AVR_IOM1284P_H_)))
#ifndef DONT_USE_UART1
class uart1_t {

	queue <char, UART1_RX_BUF> m_rx;
#ifdef UART1_ASYNC_TX
	queue <char, UART1_TX_BUF> m_tx;
#endif

public:

	void data_in(char ch)
	{
		m_rx.push(ch);
	}

#ifdef UART1_ASYNC_TX
	bool data_out(char &ch)
	{
		if(m_tx.empty())
			return false;
		ch = m_tx.pop();
		return true;
	}

	inline bool is_send()
	{
		return m_tx.empty();
	}
	
	void wait()
	{
		while(!m_tx.empty()) {}
	}
#endif //UART1_ASYNC_TX

	uint32_t init(uint32_t speed)
	{
		UCSR1A = (1<<U2X1);
		UCSR1B = ((1<<RXCIE1)|(1<<RXEN1)|(1<<TXEN1));
		UCSR1C = ((1<<UCSZ10)|(1<<UCSZ11));
		speed = uint16_t(F_CPU / (8 * speed) - 1);
		UBRR1H = ((speed&0xFF00)>>8);
		UBRR1L = (speed&0x00FF);
#ifdef RS4851_CTRL_PIN
		RS4851_CTRL_PIN::output(true);
		RS4851_CTRL_PIN::clear();
#ifdef UART1_ASYNC_TX
		UCSR1B |= (1<<TXCIE1);
#endif
#endif
		return F_CPU/((float)(8*(((UBRR1H<<8)|UBRR1L)+1)));
	}
	void write(const char &data)
	{
#ifdef UART1_ASYNC_TX
#ifdef RS4851_CTRL_PIN
		RS4851_CTRL_PIN::set();
#endif
		while(!m_tx.push(data)) {}
		UCSR1B |= (1<<UDRIE1);
#else //UART1_ASYNC_TX
		write_immediately(data);
#endif //UART1_ASYNC_TX
	}
	
	void write_immediately(const char &data)
	{
#ifdef RS4851_CTRL_PIN
		RS4851_CTRL_PIN::set();
#endif
		UCSR1A |= (1<<TXC1);
		UDR1 = data;
		while((UCSR1A & (1<<TXC1)) == 0){}
#ifdef RS4851_CTRL_PIN
		RS4851_CTRL_PIN::clear();
#endif
	}
	

	bool peek(char & data)
	{
		if(m_rx.empty())
			return false;
		data = m_rx.pop();
		return true;
	}

	char get()
	{
		char data;
		while(!peek(data)) {}
		return data;
	}
	
	inline void rx_process()
	{
		char data = UDR1;
		if((UCSR1A & (1<<FE1)) == (1<<FE1))
			return;
		data_in(data);
#ifndef DONT_USE_BOOTLOADER
#if BOOTLOADER_PORT == 1
		static const unsigned char bootSeq[] = BOOTLOADER_INIT_SEQ;
		static uint8_t state = 0;
		if (data == bootSeq[state])
		{
			if (++state == 4)
				wdt_enable(WDTO_15MS);
		}
		else
			state = 0;
#endif //BOOTLOADER_PORT
#endif //DONT_USE_BOOTLOADER
	}
	
#ifdef UART1_ASYNC_TX
	inline void tx_process()
	{
		char data;
		if(data_out(data))
			UDR1 = data;
		else
			UCSR1B &= ~(1<<UDRIE1);
	}
	
#ifdef RS4851_CTRL_PIN
	inline void tx_end_process()
	{
		RS4851_CTRL_PIN::clear();
	}
#endif
#endif //UART1_ASYNC_TX

}; 

#ifdef KUBAS_EXTERN_OBJECTS
extern uart1_t uart1;
#else
uart1_t uart1;

ISR(USART1_RX_vect)
{
	uart1.rx_process();
}
#ifdef UART1_ASYNC_TX
ISR(USART1_UDRE_vect)
{
	uart1.tx_process();
}
#ifdef RS4851_CTRL_PIN
ISR(USART1_TX_vect)
{
	uart1.tx_end_process();
}
#endif //RS4851_CTRL_PIN
#endif //UART1_ASYNC_TX
#endif //KUBAS_EXTERN_OBJECTS
#endif //DONT_USE_UART1
#endif //is usart1 available

}//end of namespace

#endif //multiple include protection

#ifndef AVRLIB_FORMAT_HPP
#define AVRLIB_FORMAT_HPP

#include <stdint.h>
#include <avr/pgmspace.h>

namespace avrlib {

template <typename Stream>
void send(Stream & s, char const * str)
{
	for (; *str != 0; ++str)
		s.write(*str);
}

template <typename Stream, typename Unsigned>
void send_hex(Stream & s, Unsigned v, uint8_t width = 0, char fill = '0')
{
	static char const digits[] = "0123456789ABCDEF";

	char buf[32];
	uint8_t i = 0;

	if (v == 0)
	{
		buf[i++] = '0';
	}
	else if (v < 0)
	{
		buf[i++] = '*';
	}
	else
	{
		for (; v != 0; v >>= 4)
		{
			buf[i++] = digits[v & 0xF];
		}
	}

	while (i < width)
		buf[i++] = fill;
	
	for (; i > 0; --i)
		s.write(buf[i - 1]);
}

template <typename Stream, typename Signed>
void send_shex(Stream & s, Signed v, uint8_t width = 0, char fill = ' ')
{
	static char const digits[] = "0123456789ABCDEF";

	char buf[32];
	uint8_t i = 0;

	bool negative = (v < 0);
	if (negative)
		v = -v;

	if (v == 0)
	{
		buf[i++] = '0';
	}
	else
	{
		for (; v != 0; v >>= 4)
		{
			buf[i++] = digits[v & 0xF];
		}
	}

	if (negative)
		buf[i++] = '-';

	while (i < width)
		buf[i++] = fill;
	
	for (; i > 0; --i)
		s.write(buf[i - 1]);
}

template <typename Stream, typename Integer>
void send_int(Stream & s, Integer v, uint8_t width = 0, char fill = ' ')
{
	char buf[32];
	uint8_t i = 0;
	bool negative = false;

	if (v == 0)
	{
		buf[i++] = '0';
	}
	else 
	{
		if (v < 0)
		{
			negative = true;
			v = -v;
		}
		
		for (; v != 0; v /= 10)
		{
			buf[i++] = (v % 10) + '0';
		}
	}
	
	if (negative)
		buf[i++] = '-';

	while (i < width)
		buf[i++] = fill;
	
	for (; i > 0; --i)
		s.write(buf[i - 1]);
}

template <typename Stream, typename T>
void send_bin(Stream & s, T const & t)
{
	char const * ptr = reinterpret_cast<char const *>(&t);
	for (uint8_t i = 0; i < sizeof t; ++i)
		s.write(ptr[i]);
}

template <typename Stream>
uint8_t readline(Stream & s, uint8_t * buffer, uint8_t len)
{
	uint8_t i = 0;

	while (i < len)
	{
		while (s.empty())
		{
			s.process_rx();
			s.process_tx();
		}

		uint8_t ch = s.read();
		buffer[i++] = ch;

		if (ch == '\n')
			break;
	}

	return i;
}

template <int N>
bool bufcmp(uint8_t const * buf, uint8_t len, char const (&pattern)[N])
{
	for (int i = 0; i < N - 1; ++i)
		if (buf[i] != pattern[i])
			return false;
	return true;
}

template <typename Stream, typename Pattern>
class format_impl
{
public:
	format_impl(Stream & out, Pattern const & pattern)
		: m_out(out), m_pattern(pattern)
	{
		this->write_literal();
	}

	format_impl & operator%(char const * str)
	{
		while (*str)
			m_out.write(*str++);
		m_pattern.pop();
		this->write_literal();
		return *this;
	}

	template <typename T>
	format_impl & operator%(T const & t)
	{
		bool hex = m_pattern.top() == 'x';
		if (hex)
			send_hex(m_out, t);
		else
			send_int(m_out, t);
		m_pattern.pop();
		this->write_literal();
		return *this;
	}

private:
	void write_literal()
	{
		bool escape = false;
		for (; !m_pattern.empty(); m_pattern.pop())
		{
			char ch = m_pattern.top();

			if (ch == '%')
			{
				if (escape)
				{
					m_out.write('%');
					m_out.write('%');
					escape = false;
				}
				else
				{
					escape = true;
				}
			}
			else
			{
				if (escape)
					break;
				m_out.write(ch);
			}
		}
	}

	Stream & m_out;
	Pattern m_pattern;
};

class string_literal_range
{
public:
	string_literal_range(char const * pattern)
		: m_pattern(pattern)
	{
	}

	bool empty() const
	{
		return *m_pattern == 0;
	}

	char top() const
	{
		return *m_pattern;
	}

	void pop()
	{
		++m_pattern;
	}

private:
	char const * m_pattern;
};

class pgm_literal_range
{
public:
	pgm_literal_range(prog_char const * first, prog_char const * last)
		: m_first(first), m_last(last)
	{
	}

	bool empty() const
	{
		return m_first == m_last;
	}

	char top() const
	{
		return pgm_read_byte(m_first);
	}

	void pop()
	{
		++m_first;
	}

private:
	prog_char const * m_first;
	prog_char const * m_last;
};

template <typename Stream>
format_impl<Stream, string_literal_range> format(Stream & out, char const * pattern)
{
	return format_impl<Stream, string_literal_range>(out, string_literal_range(pattern));
}

template <typename Stream, int N>
format_impl<Stream, pgm_literal_range> format_pgm(Stream & out, prog_char const (&pattern)[N])
{
	return format_impl<Stream, pgm_literal_range>(out, pgm_literal_range(pattern, pattern + N - 1));
}

}

#endif

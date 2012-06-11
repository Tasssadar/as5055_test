#ifndef KUBAS_FORMAT
#define KUBAS_FORMAT

#include "string.h"

#ifndef KUBAS_NUMBER_2_STRING_CHARSET
#define KUBAS_NUMBER_2_STRING_CHARSET "0123456789ABCDEF"
#endif

namespace kubas {

template <typename Integer>
bool number2string (Integer n, string &s, uint8_t align = 0,
	uint8_t base = 10, char fill = ' ',
	uint8_t division = KUBAS_DEFAULT_STRING_SIZE, char separator = ' ',
	bool Minus = true)
{
	char charset[] = KUBAS_NUMBER_2_STRING_CHARSET;
	char buf[KUBAS_DEFAULT_STRING_SIZE];
	uint8_t i = 0;
	s.clear();
	bool negative = false;
	uint8_t division_counter = division;
	if(n == 0)
		buf[i++] = charset[0];
	else
	{
		if(n < 0)
		{
			negative = true;
			n = -n;
		}
		for(; n != 0; n /= base)
		{
			if(i == (KUBAS_DEFAULT_STRING_SIZE-2))
				return false;
			buf[i++] = charset[n % base];
			if(--division_counter == 0)
			{
				buf[i++] = separator;
				division_counter = division;
			}
		}
	}
	if(Minus && negative)
		buf[i++] = '-';
	++division_counter;
	while(i < align)
	{
		if(--division_counter != 0)
			buf[i++] = fill;
		else
		{
			buf[i++] = separator;
			division_counter = division+1;
		}
	}
	if(!Minus)
	{
		if(negative)
			buf[--i] = '-';
		else
			if(fill == '0')
				buf[--i] = ' ';
			else
				buf[--i] = fill;
		++i;
	}
	for(--i; i != 255; --i)
		s += buf[i];
	return true;
}

template <typename Integer>
bool string2number(const string &s, Integer &res)
{
	bool isNegative = s[0]=='-';
	Integer n = 0;
	for(uint8_t i = isNegative?1:0; i != s.length(); ++i)
	{
		if(s[i]<'0' || s[i]>'9')
			return false;
		n = n*10+s[i]-'0';
	}
	if(isNegative)
		n = -n;
	res = n;
	return true;
}

template <typename Stream_type, typename size_type = uint8_t>
class ostream_t {

	Stream_type & Stream;
	uint8_t Align;
	uint8_t Base;
	char Fill;
	uint8_t Division;
	char Separator;
	bool Minus;//close to number (true) or before align chars (false)

public:

	ostream_t(Stream_type & stream)
		:Stream(stream), Align(0), Base(10), Fill(' '), Division(KUBAS_DEFAULT_STRING_SIZE), Separator(' '), Minus(true)
	{}

	void write(const char *str, size_type length = 0)
	{
		if(length == 0)
			for(; *str != 0; ++str)
				Stream.write(*str);
		else
			for(; length != 0; --length, ++str)
				Stream.write(*str);
	}

	void align(uint8_t n) { Align = n; }
	void base (uint8_t n) { Base = n;  }
	void fill (char n)    { Fill = n;  }
	void division (uint8_t n) { Division = n;  }
	void separator (char n)    { Separator = n;  }
	uint8_t align() { return Align; }
	uint8_t base()  { return Base;  }
	char    fill()  { return Fill;  }
	uint8_t division()  { return Division;  }
	char    separator()  { return Separator;  }
	void minus(bool m) { Minus = m; }
	bool minus() { return Minus; }
//********
	ostream_t &operator << (const char &ch)
	{
		Stream.write(ch);
		return *this;
	}
	
	ostream_t &operator << (const char *str)
	{
		for(; *str != 0; ++str)
			Stream.write(*str);
		return *this;
	}
	template <typename Integer>
	ostream_t &operator << (Integer n)
	{
		string buf;
		number2string(n, buf, Align, Base, Fill, Division, Separator, Minus);
		for(uint8_t i = 0; i != buf.length(); ++i)
			Stream.write(buf[i]);
		return *this;
	}
	ostream_t &operator << (const bool &n)
	{
		write(n?"true ":"false");
		return *this;
	}
	ostream_t &operator << (const string &str)
	{
		for(uint8_t i = 0; i != str.length(); ++i)
			Stream.write(str[i]);
		return *this;
	}

	//void wait() { Stream.wait(); }
};


template <typename input_Stream_type, typename output_Stream_type = input_Stream_type>
class istream_t {

	bool cin_isFirst;
	string cin_buf;
	bool cin_isGood;
	input_Stream_type & IStream;
	output_Stream_type * OStream;
	
public:

	istream_t(input_Stream_type & istream)
		:IStream(istream), OStream(0)
	{
		cin_isFirst = true;
		cin_isGood = true;
		cin_buf.clear();
	}
	
	istream_t(input_Stream_type & istream, output_Stream_type & ostream)
		:IStream(istream), OStream(&ostream)
	{
		cin_isFirst = true;
		cin_isGood = true;
		cin_buf.clear();
	}
	
	istream_t &operator >> (char &ch)
	{
		uint8_t i = 0;
		char temp = 0;
		while(temp != '\r')
		{
			temp = IStream.get();
			if(i == 0)
				ch = temp;
			if(temp != 8)
				++i;
			else
				if(i != 0)
					--i;
		}
		return *this;
	}

	istream_t &operator >> (string &str)
	{
		if(cin_isFirst)
		{
			char ch = IStream.get();
			cin_buf.clear();
			while(ch != '\r')
			{
				if(ch == 8)
				{
					if(cin_buf.length() != 0)
					{
						if(OStream != 0)
							OStream->write(ch);
						cin_buf.erase(cin_buf.length()-1, 1);
					}
					else
					{
						if(OStream != 0)
							OStream->write('\a');
					}					
				}
				else
				{
					cin_buf += ch;
					if(cin_buf.length() == cin_buf.max_size())
					{
						if(OStream != 0)
							OStream->write('\a');
					}					
					else
					{
						if(OStream != 0)
							OStream->write(ch);
					}						
				}
				ch = IStream.get();
			}
			if(OStream != 0)
			{
				for(const char *str = endl; *str != 0; ++str)
					OStream->write(*str);
			}
			cin_isFirst = false;
		}
		uint8_t i = cin_buf.find(' ');
		if (i == string::npos)
			cin_isFirst = true;
		str = cin_buf.substr(0, i);
		cin_buf.erase(0, i + 1);
		return *this;
	}
	istream_t &operator >> (uint8_t & n)
	{
		string s;
		if(!(*this>>s)||!string2number(s, n))
			cin_isGood = false;
		return *this;
	}
	istream_t &operator >> (int8_t & n)
	{
		string s;
		if(!(*this>>s)||!string2number(s, n))
			cin_isGood = false;
		return *this;
	}
	istream_t &operator >> (uint16_t & n)
	{
		string s;
		if(!(*this>>s)||!string2number(s, n))
			cin_isGood = false;
		return *this;
	}
	istream_t &operator >> (int16_t & n)
	{
		string s;
		if(!(*this>>s)||!string2number(s, n))
			cin_isGood = false;
		return *this;
	}
	istream_t &operator >> (uint32_t & n)
	{
		string s;
		if(!(*this>>s)||!string2number(s, n))
			cin_isGood = false;
		return *this;
	}
	istream_t &operator >> (int32_t & n)
	{
		string s;
		if(!(*this>>s)||!string2number(s, n))
			cin_isGood = false;
		return *this;
	}

	operator void const *() const { return cin_isGood?this:0; }

	void clear()
	{
		cin_isGood = true;
		cin_buf.clear();
		cin_isFirst = true;
	}

	bool is_complete() { return cin_isFirst; }

	bool peek(char &ch) { return IStream.peek(ch); }

	char get() { return IStream.get(); }
};



template <typename Stream_type, typename size_type = uint8_t>
struct iostream_t
	:ostream_t<Stream_type, size_type>, istream_t<Stream_type>
{
	iostream_t(Stream_type & stream)
		:ostream_t<Stream_type, size_type>(stream), istream_t<Stream_type>(stream, stream)
	{}
		
	iostream_t(Stream_type & stream, int)
		:ostream_t<Stream_type, size_type>(stream), istream_t<Stream_type>(stream)
	{}
};

}//end of namespace kubas

#endif

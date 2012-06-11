template <typename  e_port, uint8_t  e_pin,
		  typename rw_port, uint8_t rw_pin,
		  typename rs_port, uint8_t rs_pin,
		  typename bl_port, uint8_t bl_pin,
		  typename db_port>
class display_t {

	typedef pin< e_port,  e_pin> e;
	typedef pin<rw_port, rw_pin> rw;
	typedef pin<rs_port, rs_pin> rs;
	typedef pin<bl_port, bl_pin> bl;

	volatile uint8_t position;
	uint8_t align;
	uint8_t base;
	char fill;
	bool Display;
	bool Cursor;
	bool Brinking;
	bool Shifting;
	bool Direction;
	bool ShiftingDir;
	bool ShiftingPart;
	bool Minus;//close to number (true) or before align chars (false)
	volatile bool Running_text;
	volatile uint8_t text_length;
	volatile uint8_t text_timer;
	volatile uint8_t text_state;
	volatile uint8_t text_start;
	volatile uint8_t text_end;
	volatile uint8_t text_repeat;
	volatile char *  text_text;

public:

//FIX ME!
	uint8_t read_byte(bool data = true)
	{
		uint8_t byte = 0;
		cli();
		db_port::dir(0);
		rs::set(data);
		rw::set();
		nop();
		e::set();
		nop();
		byte = db_port::pin();
		e::clear();
		rw::clear();
		nop();
		db_port::dir(0xFF);
		sei();
		_delay_us(46);
		return byte;
	}

	void write_byte(uint8_t byte, bool data = true)
	{
		//cli();
		rs::set(data);
		e::set();
		nop();
		db_port::port(byte);
		e::clear();
		//sei();
		if(data)
		{
			_delay_us(46);
			++position;
			if(!Shifting)
			{
				if(position == 16)
				{
					setPos(64);
					//write_byte(' ');
				}
				else if(position == 80)
				{
					setPos(0);
					//write_byte(' ');
				}
			}
			//else if(position == 80)
			//	position = 0;
		}
		else if(byte < 4)
			_delay_us(1640);
		else
			_delay_us(40);
		//rs232<<data<<position<<endl;
	}

	void init()
	{
		_delay_ms(100);
		e::output(true);
		rw::output(true);
		rs::output(true);
		bl::output(true);
		rw::clear();
		db_port::dir(0xFF);
		const uint8_t bytes[] = {
			(1<<5)|(1<<4)|(1<<3),			//Function set
			(1<<4)|(1<<2),					//Cursor / Display shift
			(1<<3)|(1<<2)|(1<<1),			//on / off control
			(1<<2)|(1<<1),					//Entry mode
			(1<<1),							//Cursor at home
			(1<<0),							//Clear display
		};
		_delay_ms(200);
		for(uint8_t i = 0; i != 6; ++i)
			write_byte(bytes[i], false);
		position = 0;
		align = 4;
		base = 10;
		fill = ' ';
		Display = true;
		Cursor = true;
		Brinking = false;
		Shifting = false;
		Direction = true;
		ShiftingDir = true;
		ShiftingPart = false;
		Running_text = false;
		Minus = true;
	}

	void clear()
	{
		write_byte(1, false);
		position = 0;
		Running_text = false;
	}
	void home()
	{
		write_byte(2, false);
		position = 0;
		Running_text = false;
	}

	void setPos(uint8_t pos)
	{
		position = pos;
		write_byte((1<<7)|pos, false);
	}

	void sendChar(const char &ch)
	{
		switch(ch)
		{
			case '\r':
				if(Running_text)
				{
					write_byte(5);
					break;
				}
				if(position < 40)
					setPos(0);
				else
					setPos(64);
				break;

			case '\n':
				if(Running_text)
				{
					write_byte(6);
					break;
				}
				if(position < 40)
					setPos(position + 64);
				else
					setPos(position - 64);
				break;

			default:
				write_byte(ch);
		}
	}

	display_t &operator << (const char &ch)
	{
		sendChar(ch);
		return *this;
	}
	display_t &operator << (const char *ch)
	{
		for(;*ch!=0; ++ch)
			sendChar(*ch);
		return *this;
	}
	display_t &operator << (char *ch)
	{
		for(;*ch!=0; ++ch)
			sendChar(*ch);
		return *this;
	}
	template <typename Integer>
	display_t &operator << (Integer n)
	{
		char charset[] = "0123456789ABCDEF";
		char buf[32];
		int8_t i = 0;
		bool negative = false;

		if(n == 0)
			buf[i++] = charset[0];
		else
		{
			if(n < 0)
				negative = true;
			for(; n != 0; n /= base)
				buf[i++] = charset[(n % base)*(negative?-1:1)];
		}
		if(Minus && negative)
			buf[i++] = '-';
		while(i < align)
			buf[i++] = fill;
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
		for(; i > 0; --i)
			write_byte(buf[i-1]);
		return *this;
	}
	display_t &operator << (const bool &n)
	{
		operator <<(n?" true":"false");
		return *this;
	}

	void setAlign(uint8_t a, char w = ' ')
	{
		align = a;
		fill = w;
	}
	void setBase(uint8_t b) { base = b; }
	uint8_t getPos() { return position; }
	void display(bool a)
	{
		Display = a;
		write_byte((1<<3)|(Display<<2)|(Cursor<<1)|(Brinking<<0), false);
	}
	void cursor(bool a)
	{
		Cursor = a;
		write_byte((1<<3)|(Display<<2)|(Cursor<<1)|(Brinking<<0), false);
	}
	void brinking(bool a)
	{
		Brinking = a;
		write_byte((1<<3)|(Display<<2)|(Cursor<<1)|(Brinking<<0), false);
	}
	void shifting(bool a)
	{
		Shifting = a;
		write_byte((1<<2)|(Direction<<1)|(Shifting<<0), false);
	}
	void cursor_direction(bool a)
	{
		Direction = a;
		write_byte((1<<2)|(Direction<<1)|(Shifting<<0), false);
	}
	void shifting_direction(bool a)
	{
		ShiftingDir = a;
		write_byte((1<<4)|(ShiftingPart<<3)|(ShiftingDir<<2), false);
	}
	void displayCursor_shifting(bool a)
	{
		ShiftingPart = a;
		write_byte((1<<4)|(ShiftingPart<<3)|(ShiftingDir<<2), false);
	}
	void charracter_edit(char *binary, uint8_t pos)
	{
		uint8_t position_backup = position;
		write_byte(((1<<6)|((pos&0x07)<<3)), false);
		for(uint8_t i = 0; i != 8; ++i)
			write_byte(*(binary+i));
		setPos(position_backup);
	}
	void backlight(bool v = bl::value()) { bl::set(!v); }

	uint8_t running_text(char *Ch, uint8_t start, uint8_t end, uint8_t repeat = 0)
	{
		text_timer  = 1;
		text_state  = 0;
		text_start  = start;
		text_end    = end;
		text_text   = Ch;
		text_repeat = repeat;
		text_length = 0;
		if(repeat != 0)
			++text_repeat;
		for(char *ch = ((char*)(text_text)); *ch != 0; ++ch)
			++text_length;
		Running_text = true;
		return text_length;
	}
	void stop() { Running_text = false; }
	void refresh()
	{
		if(Running_text)
		{
			uint8_t pos = position;
			bool stop = true;
			while(stop)
			{
				stop = false;
				if(--text_timer == 0)
				{
					if(text_state++ == 0)
					{
						if(--text_repeat == 0)
						{
							Running_text = false;
							break;
						}
						if(text_repeat == 255)
							text_repeat = 0;
						text_length = 0;
						setPos(text_start);
						for(char *ch = ((char*)(text_text)); *ch != 0; ++ch)
							if(++text_length<=(text_end-text_start))
								sendChar(*ch);
						if(text_length > (text_end-text_start))
							text_length -= (text_end-text_start);
						else
						{
							while(++text_length != (text_end-text_start+1))
								sendChar(' ');
							text_length = 0;
						}
						setPos(text_start);
						text_timer = 128;
					}
					else if(text_state != (text_length + 2))
					{
						setPos(text_start);
						for(uint8_t i = 0; i != (text_end-text_start); ++i)
							sendChar(*(((char*)(text_text))+text_state+i-1));
						text_timer = 16;
					}
					else
					{
						text_state = 0;
						text_timer = 48;
					}
				}
			}
			setPos(pos);
		}
	}
	bool is_running() {	return Running_text; }
	void wait() { while(Running_text) {} }
	void minus(bool m) { Minus = m; }
	bool minus() { return Minus; }
};

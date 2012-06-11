#ifndef PWM_RESOLUTION
#define PWM_RESOLUTION 1000
#endif
template < typename in_a, typename in_b, typename en_a, typename en_b, typename OCR , const bool invert = false>
struct driver_t
{
	static void power(int16_t v, bool brakes = false)
	{
		//pc<<v<<' ';
		if(invert)
			v = -v;
		if (v <= 0)
		{
			if (!brakes)
				in_a::set();
			else
				in_a::clear();
			in_b::clear();
		}
		else
		{
			in_b::set();
			in_a::clear();
		}
		
		uint16_t abs_current = v < 0? -v: v;
		if (abs_current > PWM_RESOLUTION)
			abs_current = PWM_RESOLUTION;
		OCR::ocr(abs_current);
		//pc<<OCR::ocr()<<' ';
	}

	int16_t power() { return (in_b::get()>in_a::get()?OCR::ocr():-OCR::ocr())*(invert?-1:1); }

	void enable(bool en = true)
	{
		en_a::set(en);
		en_a::output(!en);
		en_b::set(en);
		en_b::output(!en);
	}

	static void brake()
	{
		in_a::set();
		in_b::set();
		OCR::ocr(0);
	}
};


struct ocr1a
{
	inline static uint16_t ocr() { return OCR1A; }
	inline static void ocr(uint16_t v) { OCR1A = v; }
};

struct ocr1b
{
	inline static uint16_t ocr() { return OCR1B; }
	inline static void ocr(uint16_t v) { OCR1B = v; }
};

struct ocr1c
{
	inline static uint16_t ocr() { return OCR1C; }
	inline static void ocr(uint16_t v) { OCR1C = v; }
};

struct ocr3a
{
	inline static uint16_t ocr() { return OCR3A; }
	inline static void ocr(uint16_t v) { OCR3A = v; }
};

struct ocr3b
{
	inline static uint16_t ocr() { return OCR3B; }
	inline static void ocr(uint16_t v) { OCR3B = v; }
};

struct ocr3c
{
	inline static uint16_t ocr() { return OCR3C; }
	inline static void ocr(uint16_t v) { OCR3C = v; }
};

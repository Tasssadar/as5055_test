#ifndef AVRLIB_PIN_HPP
#define AVRLIB_PIN_HPP

namespace avrlib {
	
struct pin_base
{
	virtual void set(bool value = true) {}

	virtual void clear() {}

	virtual bool get() { return false; }

	virtual bool value() { return false; }

	virtual void output(bool value) {}

	virtual bool output() { return false; }

	virtual void make_input() {}
	virtual void make_low() {}
	virtual void make_high() {}
	virtual void set_value(bool value) {}
};

template <typename Port, int Pin>
struct pin :pin_base
{
	void set(bool value = true)
	{
		if (value)
			Port::port(Port::port() | (1<<Pin));
		else
			Port::port(Port::port() & ~(1<<Pin));
	}

	void clear() { set(false); }

	bool get() { return (Port::port() & (1<<Pin)) != 0; }

	bool value() { return (Port::pin() & (1<<Pin)) != 0; }

	void output(bool value)
	{
		if (value)
			Port::dir(Port::dir() | (1<<Pin));
		else
			Port::dir(Port::dir() & ~(1<<Pin));
	}

	bool output() { return (Port::dir() & (1<<Pin)) != 0; }

	void make_input() { output(false); clear(); }
	void make_low() { clear(); output(true); }
	void make_high() { set(); output(true); }
	void set_value(bool value) { set(value); }
};

template <typename Port, int Pin>
struct _pin :pin_base
{
	void set(bool value = true)
	{
		if (!value)
			Port::port(Port::port() | (1<<Pin));
		else
			Port::port(Port::port() & ~(1<<Pin));
	}

	void clear() { set(false); }

	bool get() { return (Port::port() & (1<<Pin)) == 0; }

	bool value() { return (Port::pin() & (1<<Pin)) == 0; }

	void output(bool value)
	{
		if (value)
			Port::dir(Port::dir() | (1<<Pin));
		else
			Port::dir(Port::dir() & ~(1<<Pin));
	}

	bool output() { return (Port::dir() & (1<<Pin)) != 0; }

	void make_input() { output(false); clear(); }
	void make_low() { clear(); output(true); }
	void make_high() { set(); output(true); }
	void set_value(bool value) { set(value); }
};

}

#endif

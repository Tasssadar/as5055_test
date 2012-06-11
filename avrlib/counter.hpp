#ifndef AVRLIB_TIMER_HPP
#define AVRLIB_TIMER_HPP

#include <stdint.h>
#include "timer_base.hpp"

namespace avrlib {

template <typename Timer, typename Time = uint32_t>
struct counter
{
	typedef Timer timer_type;
	typedef Time time_type;

	template <uint32_t v>
	struct us : timer_type::template us<v>
	{
	};

	counter()
		: m_overflows(0)
	{
		timer_type::value(0);
	}

	template <typename Prescaler>
	explicit counter(Prescaler const & prescaler)
		: m_overflows(0)
	{
		timer_type::value(0);
		this->enable(prescaler);
	}

	~counter()
	{
		this->disable();
	}

	void enable(timer_clock_source prescaler)
	{
		timer_type::clock_source(prescaler);
		timer_type::tov_interrupt(true);
	}

	void disable()
	{
		timer_type::tov_interrupt(false);
		timer_type::clock_source(timer_no_clock);
	}

	time_type value() const
	{
		typename timer_type::time_type time = timer_type::value();
		uint16_t overflows;

		for (;;)
		{
			// This is needed for correctness in the case of interrupt congestion.
			// Perhaps we could afford to cli, process and sei, but that would screw up
			// interrupt priorities.
			while (timer_type::overflow()) {}

			overflows = static_cast<uint16_t volatile const &>(m_overflows);
			typename timer_type::time_type new_time = timer_type::value();

			if (new_time >= time)
				return ((time_type)overflows << timer_type::value_bits) | new_time;

			time = new_time;
		}
	}

	void tov_interrupt() const
	{
		++static_cast<uint16_t volatile &>(m_overflows);
	}

private:
	mutable uint16_t m_overflows;
};

}

#endif

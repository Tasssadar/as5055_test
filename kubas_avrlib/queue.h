#ifndef kubaslib_queue
#define kubaslib_queue
namespace kubas
{

template <typename T, uint16_t _size>
class queue {
	T m_data [_size];
	volatile uint16_t m_read;
	volatile uint16_t m_write;
	
public:

	bool empty() const
	{
		if (m_write == m_read)
			return true;
		return false;
	}
	
	uint16_t size() const
	{
		if (m_write >= m_read)
			return m_write - m_read;
		return _size - m_read + m_write;
	}
	
	inline const uint16_t& max_size() const
	{
		return _size;
	}
	
	bool push (T data)
	{
		m_data[m_write] = data;
		if (++m_write == _size)
			m_write = 0;
		if (empty())
		{
			if (++m_read == _size)
				m_read = 0;
			return false;
		}
		return true;
	}
	
	T pop ()
	{
		if (empty())
			return m_data[m_read];
		uint16_t read = m_read;
		if (++m_read == _size)
			m_read = 0;
		return m_data[read];
	}
	
	const T& front() const
	{
		return m_data[m_read];
	}
	
	inline void clear()
	{
		m_read = 0;
		m_write = 0;
	}
};

}//end of namespace kubas
#endif

#ifndef kubaslib_deque
#define kubaslib_deque
namespace kubas
{

template <typename T, uint16_t _size>
class deque {
	T m_data [_size];
	volatile uint16_t sp;
	volatile uint16_t ep;

public:

//constructor

	deque ()
		:sp(0), ep(0)
	{}

//pointers

	inline T* begin()
	{
		return m_data + ep;
	}
	
	inline const T* begin() const
	{
		return m_data + ep;
	}
	
	inline T* end()
	{
		return m_data + sp;
	}
	
	inline const T* end() const
	{
		return m_data + sp;
	}
	
//capacity

	inline uint16_t size() const
	{
		if (sp >= ep)
			return sp - ep;
		return _size - ep + sp;
	}
	
	inline uint16_t max_size() const
	{
		return _size;
	}

	inline bool empty()
	{
		if (sp == ep)
			return true;
		return false;
	}

//element access

	inline const T& operator[](uint16_t index) const
	{
		return m_data[(sp + index) >= _size? (index + sp - _size) : (sp + index)];
	}
	
	inline T& operator[](uint16_t index)
	{
		return m_data[(sp + index) >= _size? (index + sp - _size) : (sp + index)];
	}
	
	const T& at(uint16_t index) const
	{
		if (index < size())
			return m_data[(sp + index) >= _size? (index + sp - _size) : (sp + index)];
		index = size();
		return m_data[(sp + index) >= _size? (index + sp - _size) : (sp + index)];
	}

	T& at(uint16_t index)
	{
		if (index < size())
			return m_data[(sp + index) >= _size? (index + sp - _size) : (sp + index)];
		index = size();
		return m_data[(sp + index) >= _size? (index + sp - _size) : (sp + index)];
	}
	
	inline const T& front() const
	{
		return m_data[ep];
	}
	
	inline T& front()
	{
		return m_data[ep];
	}
	
	inline const T& back() const
	{
		return m_data[sp == 0? (_size - 1) : sp];
	}
	
	inline T& back()
	{
		return m_data[sp == 0? (_size - 1) : sp];
	}
	
//modifiers

	template <typename InputPointer>
	void assign(InputPointer first, InputPointer last)
	{
		clear();
		for(; first != last; ++first)
			push_back(*first);
	}
	
	void assign(uint16_t n, const T& u)
	{
		clear();
		for(uint16_t i = 0; i != n; ++i)
			push_back(u);
	}
	
	inline deque<T, _size>& operator = (const deque<T, _size>& x)
	{
		assign(x.begin(), x.end());
	}
	
	bool push_back (T data)
	{
		m_data[sp] = data;
		if (++sp == _size)
			sp = 0;
		if (empty())
		{
			if (++ep == _size)
				ep = 0;
			return false;
		}
		return true;
	}
	
	bool push_front (T data)
	{
		if(ep == 0)
			ep = _size;
		m_data[--ep] = data;
		if (empty())
		{
			if (sp == 0)
				sp = _size;
			--sp;
			return false;
		}
		return true;
	}
	
	T pop_back ()
	{
		if(empty())
			return m_data[sp];
		if(sp == 0)
			sp = size;
		return m_data[--sp];
	}

	T pop_front ()
	{
		if (empty())
			return m_data[ep];
		uint16_t read = ep;
		if (++ep == _size)
			ep = 0;
		return m_data[read];
	}
	
	inline void clear()
	{
		sp = 0;
		ep = 0;
	}
};

}//end of namespace kubas
#endif
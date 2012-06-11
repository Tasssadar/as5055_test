#ifndef __KUBAS_PAIR_H_
#define __KUBAS_PAIR_H_

namespace kubas {

template <class T1, class T2>
struct pair
{
	typedef T1 first_type;
	typedef T2 second_type;

	T1 first;
	T2 second;
	pair() : first(T1()), second(T2()) {}
	pair(const T1& x, const T2& y) : first(x), second(y) {}
	template <class U, class V>
	pair (const pair<U,V> &p) : first(p.first), second(p.second) { }
};

template <class T1,class T2>
pair<T1,T2> make_pair (T1 x, T2 y)
{
	return ( pair<T1,T2>(x,y) );
}

}//end of namespace kubas

#endif
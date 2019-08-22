#pragma once
#include <vector>
#include <type_traits>
#include <cstdlib>
#include <array>
#include <numeric>
#include <cassert> 

template <size_t N>
using vector_size = std::array<size_t, N>;

namespace impl
{
	template<typename Arg>
	inline static Arg product(Arg arg)
	{
		return arg;
	}

	template<typename FirstArg, typename ... Args>
	inline FirstArg product(FirstArg first, Args ... args)
	{
		return product(args...) * first;
	}

	template<typename FirstIndex, typename ... Indexes>
	inline static size_t index(const size_t *coefs, FirstIndex first)
	{
		return *coefs * first + *(coefs + 1);
	}

	template<typename FirstIndex, typename ... Indexes>
	inline static size_t index(const size_t *coefs, FirstIndex first, Indexes ... indexes)
	{
		return *coefs * first + index(coefs + 1, indexes...);
	}

	template<typename ...Args>
	void ignore(Args ...) { ; }

	template<typename ...Args>
	bool allPositive(Args ... args)
	{
		bool positive(true);
		ignore(positive = positive && args >= 0 ...);
		return positive;
	};

	inline bool checkIndex(const size_t *){ return true; }

	template<typename FirstIndex, typename ... Indexes>
	inline static bool checkIndex(const size_t *sizes, FirstIndex first, Indexes ... indexes)
	{
		// May be I should to use additional checks for signed types because of UB
		return size_t(first) < *sizes && checkIndex(sizes + 1, indexes...);
	}

	template<typename FirstArg>
	inline void calcCoefficients(size_t *arr, FirstArg)
	{
		*arr = 1;
	}

	template<typename FirstArg, typename SecondArg, typename ... Args>
	inline void calcCoefficients(size_t *arr, FirstArg, SecondArg second, Args ... args)
	{
		calcCoefficients(arr + 1, second, args...);
		*arr = *(arr + 1) * second;
	}

	template <size_t N>
	inline void calcCoefficients(size_t *arr, const size_t *args)
	{
		*(arr + N - 1) = 1;

		for (int n = N - 2; n >= 0; --n)
		{
			*(arr + n) = *(arr + n + 1) * *(args + n + 1);
		}
	}

	template<class First, class ...Args> struct AllNumeric
	{
		const static auto value = std::is_integral<First>::value && AllNumeric<Args...>::value;
	};

	template<class Arg> struct AllNumeric<Arg>
	{
		const static auto value = std::is_integral<Arg>::value;
	};

	template<class ElementType, int numDims> class VectorSlice;
	
	template<class ElementType, int numCoords, int numDimsSource> struct DerefIter
	{
		std::array<size_t, numCoords> index;
		VectorSlice<ElementType, numDimsSource> value;
	};

	template<class ElementType, int numCoords> struct DerefIter<ElementType, numCoords, numCoords>
	{
		DerefIter(const std::array<size_t, numCoords> &i, const ElementType &v)
			: index(i), value(v) {}
		const std::array<size_t, numCoords> &index;
		const ElementType &value;
	};

	// STL compatible iterator
	template<class T, int numCoords>
	class ElemIter
	{
		typedef ElemIter<T, numCoords> ThisType;
	public:
		ElemIter(const std::array<size_t, numCoords> &from, 
			const std::array<size_t, numCoords> &to,
			const std::array<size_t, numCoords> &cur,
			VectorSlice<T, numCoords> data)
			: m_from(from), m_to(to),
			  m_current_pos(cur), m_data(data)
		{
			for(int i = 0; i != numCoords; ++i)
			{
				if(from[i] < to[i]) m_delta[i] = 1;
				else m_delta[i] = -1;
			}
		}

		ThisType &operator++()
		{
			int i = numCoords - 1;

			while(i != -1)
			{
				m_current_pos[i] += m_delta[i];
				if(m_current_pos[i] == m_to[i])
				{
					m_current_pos[i] = m_from[i];
					--i;
				}
				else break;
			}
			if(i == -1) m_current_pos = m_to;

			return *this;
		}

		ThisType operator++(int)
		{
			auto tmp = *this;
			++*this;
			return tmp;
		}

		bool operator==(const ThisType &other) const
		{
			return m_current_pos == other.m_current_pos && 
				m_data.data == other.m_data.data &&
				m_from == other.m_from && 
				m_to == other.m_to;
		}

		bool operator!=(const ThisType &other) const
		{
			return !(*this == other);
		}

		DerefIter<T, numCoords, numCoords> operator*()
		{
			return DerefIter<T, numCoords, numCoords>(m_current_pos, deref_impl(std::make_index_sequence<numCoords>()));
		}

	private:
		std::array<size_t, numCoords> m_from;
		std::array<size_t, numCoords> m_to;
		std::array<size_t, numCoords> m_current_pos;
		std::array<signed char, numCoords> m_delta;

		VectorSlice<T, numCoords> m_data;

		template<size_t ...I>
		T &deref_impl(std::index_sequence<I...>)
		{
			return m_data(m_current_pos[I]...);
		}
	};

	template<class T, int ...IS>
	class Indexer;

	template <int V, int... Tail> constexpr bool has_v = false;
	template <int V, int F, int... Tail>
	constexpr bool has_v<V, F, Tail...> = (V == F) || has_v<V, Tail...>;

	template <int... I> constexpr bool distinct = true;
	template <int F, int S, int... Tail>
	constexpr bool distinct<F, S, Tail...> = !has_v<F, S, Tail...> && distinct<S, Tail...>;

	template <int I, int... Tail> constexpr int min = I;
	template <int F, int S, int... Tail>
	constexpr int min<F, S, Tail...> = min<(F < S ? F : S), Tail...>;

	template <int I, int... Tail> constexpr int max = I;
	template <int F, int S, int... Tail>
	constexpr int max<F, S, Tail...> = max<(F > S ? F : S), Tail...>;

	template<int N, int ...I> constexpr bool valid_index_set = min<I...> >= 0 && max<I...> < N && distinct<I...>;

	template<int...Nums> bool has_v_fun(int a)
	{
		for(int x : {Nums...})
		{
			if(x == a) return true;
		}
		return false;
	}


	template<class ElementType, int numDims> class VectorSlice {
		friend class ElemIter<ElementType, numDims>;
		
		template<class T, int N>
		friend class VectorSlice;

		template<class T, int ... IS>
		friend class Indexer;

	public:
		VectorSlice() : coefs{0}, sizes{0}
		{
		}

		template<typename ... Indexes>
		inline ElementType& operator()(Indexes ... indexes)
		{
			static_assert(impl::AllNumeric<Indexes...>::value, "Parameters type is invalid");
			static_assert(sizeof...(indexes) == numDims, "Parameters count is invalid");
			
			return data[getIndex(indexes ...)];
		}

		template<typename ... Indexes>
		inline const ElementType& operator()(Indexes ... indexes) const
		{
			static_assert(impl::AllNumeric<Indexes...>::value, "Parameters type is invalid");
			static_assert(sizeof...(indexes) == numDims, "Parameters count is invalid");

			return data[getIndex(indexes ...)];
		}

		template<typename ... Indexes>
		inline ElementType& at(Indexes ... indexes)
		{
			static_assert(impl::AllNumeric<Indexes...>::value, "Parameters type is invalid");
			static_assert(sizeof...(indexes) == numDims, "Parameters count is invalid");

			if(!impl::checkIndex(indexes...)) throw std::out_of_range("One or more indexes are invalid");

			return data[getIndex(indexes ...)];
		}

		template<typename ... Indexes>
		inline const ElementType& at(Indexes ... indexes) const
		{
			static_assert(impl::AllNumeric<Indexes...>::value, "Parameters type is invalid");
			static_assert(sizeof...(indexes) == numDims, "Parameters count is invalid");

			if(!impl::checkIndex(indexes...)) throw std::out_of_range("One or more indexes are invalid");

			return data[getIndex(indexes ...)];
		}

		template<int ...IS> Indexer<ElementType, IS...> get_indexer()
		{
			return Indexer<ElementType, IS...>(*this);
		}
		template<int ...NumDims> Indexer<ElementType> get_indexer() const;

		inline size_t size(const int numberDims) const
		{
			assert((numberDims - 1) <= numDims && (numberDims - 1) >= 0 && "Parameters count is invalid");

			return sizes[numberDims - 1];
		}

		inline const vector_size<numDims>& size() const
		{
			return sizes;
		}

		template<int...Indexes, class ...Args>
		VectorSlice<ElementType, numDims - sizeof...(Indexes)> fix(Args ...c_index)
		{
			const int new_dim = numDims - sizeof...(Indexes);
			static_assert(sizeof...(Indexes) == sizeof...(Args), "Indexes and template parameters count do not match");
			static_assert(valid_index_set<numDims, Indexes...>, "Invalid index set");
			static_assert(AllNumeric<Args...>::value, "Invalid arguments");

			std::array<size_t, sizeof...(Indexes)> template_index{Indexes...};
			std::array<size_t, sizeof...(Indexes)> args{size_t(c_index)...};
			
			std::array<size_t, new_dim + 1> new_coefs;
			new_coefs[new_dim] = coefs[numDims];

			std::array<size_t, new_dim> new_sizes;
			for(int i = 0, j = 0; i < numDims; ++i)
			{
				if(!has_v_fun<Indexes...>(i))
				{
					new_sizes[j] = sizes[i];
					new_coefs[j] = coefs[i];
					++j;
				}
			}
			for(size_t i = 0; i != sizeof...(Indexes); ++i)
			{
				if(args[i] >= sizes[template_index[i]]) throw std::invalid_argument("One or more index too large");
				new_coefs[new_dim] += coefs[template_index[i]] * args[i];
			}

			VectorSlice<ElementType, new_dim> res;
			res.reset(new_coefs, new_sizes, data);
			return res;
		}

	/*	template<int...IndexNumber>
		VectorSlice<T, numDims - sizeof...(IndexNumber)> slice() const
		{

		}*/

	protected:
		void reset(const std::array<size_t, numDims + 1> &acoefs,
			const std::array<size_t, numDims> &asizes, ElementType *adata)
		{
			coefs = acoefs;
			sizes = asizes;
			data = adata;
		}

	private:
		std::array<size_t, numDims + 1> coefs;
		std::array<size_t, numDims> sizes;

		ElementType *data;

		template<typename ... Indexes>
		size_t getIndex(Indexes ... indexes)
		{
			assert(impl::checkIndex(sizes.data(), indexes...) && "Indexes is invalid.");

			auto index = impl::index(coefs.data(), indexes...);

			return index;
		}
	};

	// IS - Index Sequense
	template<class T, int ... IS>
	class Indexer
	{
		constexpr static int N = sizeof...(IS);
	public:
		Indexer(VectorSlice<T, N> &source)
			: m_source(source)
		{}

		ElemIter<T, N> begin()
		{
			std::array<size_t, N> from;
			from.fill(0);
			
			auto new_data = m_source;
			new_data.coefs = {new_data.coefs[IS]..., new_data.coefs[N]};
			new_data.sizes = {new_data.sizes[IS]...};

			std::array<size_t, N> to = m_source.size();

			ElemIter<T, N> it(from, new_data.size(), // From and to
				from, // current position
				new_data);
			return it;
		}

		ElemIter<T, N> end()
		{
			std::array<size_t, N> from;
			from.fill(0);

			auto new_data = m_source;
			new_data.coefs = {new_data.coefs[IS]..., new_data.coefs[N]};
			new_data.sizes = {new_data.sizes[IS]...};

			std::array<size_t, N> to = m_source.size();
			ElemIter<T, N> it(from, new_data.size(), // From and to
				{to[IS]...}, // Current position
				new_data);
			return it;
		}

		Indexer &rev(int) {return *this;}
	private:
		VectorSlice<T, N> &m_source;

		static_assert(valid_index_set<N, IS...>, "Index set must be a permutation");
	};
}

template<typename ElementType, size_t numDims>
class vector_n : public impl::VectorSlice<ElementType, numDims>
{
	typedef impl::VectorSlice<ElementType, numDims> Base;
public:
	vector_n() 
	{
		// Do something
	}

	template<typename ... Sizes>
	vector_n(Sizes ... sizes)
		: data(impl::product(sizes ...))
	{
		static_assert(sizeof...(sizes) == numDims, "Parameters count is invalid");
		static_assert(impl::AllNumeric<Sizes...>::value, "Parameters type is invalid");

		if(!impl::allPositive(sizes ...)) throw std::invalid_argument("All dimensions must be positive");

		std::array<size_t, numDims + 1> coefs;
		coefs[numDims] = 0;

		impl::calcCoefficients(coefs.data(), sizes ...);
		impl::VectorSlice<ElementType, numDims>::reset(coefs, {size_t(sizes)...}, data.data());
	}

	inline void resize(const vector_size <numDims> &sizesDims)
	{
		data.resize(std::accumulate(sizesDims.begin(), sizesDims.end(), 1, std::multiplies<size_t>()));
		std::array<size_t, numDims> sizes = sizesDims;
		std::array<size_t, numDims + 1> coefs;
		coefs[numDims] = 0;

		impl::calcCoefficients<numDims>(coefs.data(), sizesDims.data());
		reset(coefs, sizes, data.data());
	}

	template<typename ... Sizes>
	inline void resize(Sizes ... sizesDims)
	{
		resize({sizesDims...});
	}

	inline void clear()
	{
		std::vector<ElementType>().swap(data);
	}

	typename std::vector<ElementType>::iterator begin()
	{
		return data.begin();
	}

	typename std::vector<ElementType>::iterator end()
	{
		return data.end();
	}

	typename std::vector<ElementType>::const_iterator begin() const
	{
		return data.begin();
	}

	typename std::vector<ElementType>::const_iterator end() const
	{
		return data.end();
	}

	inline std::vector<ElementType>& getData()
	{
		return data;
	}

	template<typename ... Indexes>
	inline bool existData(Indexes ... indexes) const
	{
		return impl::checkIndex(Base::sizes.data(), indexes...);
	}

private:

	std::vector<ElementType> data;
};

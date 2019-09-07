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

	size_t index(const size_t *coefs)
	{
		return *coefs;
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

	template<class ...Args> struct AllNumeric
	{
		const static auto value = true;
	};

	template<class First, class ...Args> struct AllNumeric<First, Args...>
	{
		const static auto value = std::is_integral<First>::value && AllNumeric<Args...>::value;
	};

	template<class ElementType, int numDims> class VectorSlice;

	template<class ElementType, int numDims>
	using MoveOutConst = std::conditional_t<std::is_const_v<ElementType>, 
		const VectorSlice<std::remove_const_t<ElementType>, numDims>,
		VectorSlice<ElementType, numDims>>;

	template<class ElementType, int numCoords, int numDimsSlice> struct DerefIter
	{
		DerefIter(const std::array<size_t, numCoords> &i, 
			MoveOutConst<ElementType, numDimsSlice> &v)
			: index(i), value(v) {}
		const std::array<size_t, numCoords> &index;
		MoveOutConst<ElementType, numDimsSlice> &value;
	};

	template<class ElementType, int numCoords> struct DerefIter<ElementType, numCoords, 0>
	{
		DerefIter(const std::array<size_t, numCoords> &i, const ElementType &v)
			: index(i), value(v) {}
		const std::array<size_t, numCoords> &index;
		const ElementType &value;
	};

	// STL compatible iterator
	template<class T, int numDimsSource, int numCoords>
	class ElemIter
	{
		typedef ElemIter<T, numDimsSource, numCoords> ThisType;
		typedef MoveOutConst<T, numDimsSource> SourceType;

		const static int numDimsSlice = numDimsSource - numCoords;
	public:

		template<int ...IS>
		ElemIter(const std::array<size_t, numCoords> &from, 
			const std::array<size_t, numCoords> &to,
			const std::array<size_t, numCoords> &cur,
			const std::array<size_t, numCoords> &coefs,
			SourceType &source,
			std::integer_sequence<int, IS...>)
			: m_from(from), m_to(to),
			  m_current_pos(cur)
		{
			static_assert(sizeof...(IS) == numCoords, "Number of indexes must be equal to numCoords");

			for(int i = 0; i != numCoords; ++i)
			{
				if(from[i] < to[i]) m_delta[i] = 1;
				else m_delta[i] = -1;

				m_delta1[i] = coefs[i] * m_delta[i];
				m_delta2[i] = (from[i] - to[i]) * coefs[i];
			}
			if(cur != to) update_mdata<IS...>(source, std::make_integer_sequence<int, numCoords>());
			else 
			{
				// Hack until const methods aren't implemented
				m_data.reset({}, {}, const_cast<std::remove_const_t<T>*>(source.data));
			}
		}

		ThisType &operator++()
		{
			int i = numCoords - 1;

			while(i != -1)
			{
				m_current_pos[i] += m_delta[i];
				m_data.coefs[numDimsSlice] += m_delta1[i];
				if(m_current_pos[i] == m_to[i])
				{
					m_current_pos[i] = m_from[i];
					m_data.coefs[numDimsSlice] += m_delta2[i];

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

		DerefIter<T, numCoords, numDimsSlice> operator*()
		{
			return DerefIter<T, numCoords, numDimsSlice>(m_current_pos, deref_impl(std::make_integer_sequence<int, numCoords>()));
		}

	private:
		// Iterator parameters
		std::array<size_t, numCoords> m_from;
		std::array<size_t, numCoords> m_to;
		std::array<size_t, numCoords> m_current_pos;
		std::array<signed char, numCoords> m_delta;

		// Additional members, just for performance
		VectorSlice<std::remove_const_t<T>, numDimsSlice> m_data;
		std::array<size_t, numCoords> m_delta1;
		std::array<size_t, numCoords> m_delta2;

		template<int ...I, int ...A>
		void update_mdata(SourceType &source, std::integer_sequence<int, A...>)
		{
			m_data = source.template fix<I...>(m_current_pos[A]...);
		}

		template<int ...I>
		inline std::enable_if_t<sizeof...(I) == numDimsSource, T&> deref_impl(std::integer_sequence<int, I...>)
		{
			return m_data.data[m_data.coefs[0]];
		}

		template<int ...I>
		inline std::enable_if_t<
			sizeof...(I) != numDimsSource, 
			VectorSlice<std::remove_const_t<T>, numDimsSlice>&> deref_impl(std::integer_sequence<int, I...>)
		{
			return m_data;
		}
	};

	template<class T, int N, int ...IS>
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
		
		template<class ElementType, int numDims, int N>
		friend class ElemIter;
		
		template<class T, int N>
		friend class VectorSlice;

		template<class T, int N, int ... IS>
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

		template<int ...IS> Indexer<ElementType, numDims, IS...> get_indexer_mut()
		{
			return Indexer<ElementType, numDims, IS...>(*this);
		}

		template<int ...IS> Indexer<const ElementType, numDims, IS...> get_indexer() const
		{
			return Indexer<const ElementType, numDims, IS...>(*this);
		}

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

		template<int...Indexes, class ...Args>
		VectorSlice<ElementType, numDims - sizeof...(Indexes)> fix(Args ...c_index) const
		{
			// Just a copy-paste
			const int new_dim = numDims - sizeof...(Indexes);
			static_assert(sizeof...(Indexes) == sizeof...(Args), "Indexes and template parameters count do not match");
			static_assert(valid_index_set<numDims, Indexes...>, "Invalid index set");
			static_assert(AllNumeric<Args...>::value, "Invalid arguments");

			std::array<size_t, sizeof...(Indexes)> template_index{Indexes...};
			std::array<size_t, sizeof...(Indexes)> args{size_t(c_index)...};

			std::array<size_t, new_dim + 1> new_coefs;
			new_coefs[new_dim] = coefs[numDims];

			std::array<size_t, new_dim> new_sizes;
			for (int i = 0, j = 0; i < numDims; ++i)
			{
				if (!has_v_fun<Indexes...>(i))
				{
					new_sizes[j] = sizes[i];
					new_coefs[j] = coefs[i];
					++j;
				}
			}
			for (size_t i = 0; i != sizeof...(Indexes); ++i)
			{
				if (args[i] >= sizes[template_index[i]]) throw std::invalid_argument("One or more index too large");
				new_coefs[new_dim] += coefs[template_index[i]] * args[i];
			}

			VectorSlice<ElementType, new_dim> res;
			res.reset(new_coefs, new_sizes, data);
			return res;
		}

	protected:
		void reset(const std::array<size_t, numDims + 1> &acoefs,
			const std::array<size_t, numDims> &asizes, ElementType *adata)
		{
			coefs = acoefs;
			sizes = asizes;
			data = adata;
		}

		void set_buf(ElementType *ptr)
		{
			data = ptr;
		}

	private:
		std::array<size_t, numDims + 1> coefs;
		std::array<size_t, numDims> sizes;

		ElementType *data;

		template<typename ... Indexes>
		size_t getIndex(Indexes ... indexes) const
		{
			assert(impl::checkIndex(sizes.data(), indexes...) && "Indexes is invalid.");

			auto index = impl::index(coefs.data(), indexes...);

			return index;
		}
	};

	// IS - Index Sequense
	template<class T, int N, int ... IS>
	class Indexer
	{
	public:
		typedef std::conditional_t<std::is_const_v<T>,
			const VectorSlice<std::remove_const_t<T>, N>, 
			VectorSlice<T, N>> SourceType;
		typedef ElemIter<T, N, sizeof...(IS)> IteratorType;

		Indexer(SourceType &source)
			: m_source(source)
		{
			from.fill(0);
			to = m_source.size();
		}

		IteratorType begin()
		{
			IteratorType it({from[IS]...}, {m_source.size()[IS]...}, // From and to
				{from[IS]...}, // current position
				{m_source.coefs[IS]...},
				m_source, std::integer_sequence<int, IS...>());
			return it;
		}

		IteratorType end()
		{
			IteratorType it({from[IS]...}, {m_source.size()[IS]...}, // From and to
				{to[IS]...}, // current position
				{m_source.coefs[IS]...},
				m_source, std::integer_sequence<int, IS...>());
			return it;
		}

		Indexer &rev(int) {return *this;}
	private:
		SourceType &m_source;

		std::array<size_t, N> from;
		std::array<size_t, N> to;

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

	vector_n(const vector_n &other)
		: Base(other), data(other.data)
	{
		Base::set_buf(data.data());
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

	vector_n &operator=(const vector_n &other)
	{
		if(&other != this)
		{
			data = other.data;
			// Simple assignment for the base class
			*static_cast<Base*>(this) = static_cast<const Base&>(other);

			// But special for the pointer
			Base::set_buf(data.data());
		}
		return *this;
	}

private:

	std::vector<ElementType> data;
};

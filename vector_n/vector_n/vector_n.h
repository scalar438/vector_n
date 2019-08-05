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
	inline static size_t index(const size_t *dims, FirstIndex first)
	{
		return *dims * first;
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
		return first < size_t(*sizes) && checkIndex(sizes + 1, indexes...);
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

	template<class T>
	class Indexer;

	template<class ElementType, int numDims> class VectorGeneral
	{
	public:
		VectorGeneral() : coefs{0}, sizes{0}
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

		template<int ...NumDims> Indexer<ElementType> get_indexer();
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
			bool f = impl::checkIndex(sizes.data(), indexes...);
			assert(f);

			auto index = impl::index(coefs.data(), indexes...);

			return index;
		}
	};

	template<class T>
	class Indexer
	{
		Indexer(T *data);
		void begin(){}

		void end(){}

		void rev();
	};
}

template<typename ElementType, size_t numDims>
class vector_n : public impl::VectorGeneral<ElementType, numDims>
{
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
		impl::VectorGeneral<ElementType, numDims>::reset(coefs, {size_t(sizes)...}, data.data());
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

private:

	std::vector<ElementType> data;
};

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
	inline static FirstIndex index(const size_t *sizes, const size_t *dims, FirstIndex first)
	{
		return *dims * first;
	}

	template<typename FirstIndex, typename ... Indexes>
	inline static FirstIndex index(const size_t *sizes, const size_t *dims, FirstIndex first, Indexes ... indexes)
	{
		return *dims * first + index(sizes, dims + 1, indexes...);
	}

	template<typename FirstIndex, typename ... Indexes>
	inline static void checkIndex(const size_t *size, FirstIndex first)
	{
		assert(size_t(*size) < *size && "Index is invalid.");
	}

	template<typename FirstIndex, typename ... Indexes>
	inline static void checkIndex(const size_t *sizes, FirstIndex first, Indexes ... indexes)
	{
		checkIndex(sizes, first);
		checkIndex(sizes + 1, indexes...);
	}

	template<typename FirstArg>
	inline void calcCoefficients(size_t *arr, FirstArg first)
	{
		*arr = 1;
	}

	template<typename FirstArg, typename SecondArg, typename ... Args>
	inline void calcCoefficients(size_t *arr, FirstArg first, SecondArg second, Args ... args)
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

	template<typename ...Args>
	void ignore(Args ...) { ; }

	template<typename ...Args>
	bool isPositive(Args ... args)
	{
		bool positive(true);
		ignore(positive = positive && args >= 0 ...);
		return positive;
	};
}

template<typename ElementType, size_t numDims>
class vector_n
{
public:
	vector_n() {};

	template<typename ... Sizes>
	vector_n(Sizes ... sizes)
		: data(impl::product(sizes ...))
		, sizes{ size_t(sizes)... }
	{
		static_assert(sizeof...(sizes) == numDims, "Parameters count is invalid");
		static_assert(impl::AllNumeric<Sizes...>::value, "Parameters type is invalid");

		assert(impl::isPositive(sizes ...));

		impl::calcCoefficients(dims.data(), sizes ...);
	}

	inline void resize(const vector_size <numDims> &sizesDims)
	{
		data.resize(std::accumulate(sizesDims.begin(), sizesDims.end(), 1, std::multiplies<size_t>()));
		sizes = sizesDims;

		impl::calcCoefficients<numDims>(dims.data(), sizesDims.data());
	}

	template<typename ... Sizes>
	inline void resize(Sizes ... sizesDims)
	{
		static_assert(sizeof...(sizesDims) == numDims, "Parameters count is invalid");

		data.resize(impl::product(sizesDims ...));
		sizes = { size_t(sizesDims)... };
		impl::calcCoefficients(dims.data(), sizesDims ...);
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

		return data[getIndex(indexes ...)];
	}

	template<typename ... Indexes>
	inline const ElementType& at(Indexes ... indexes) const
	{
		static_assert(impl::AllNumeric<Indexes...>::value, "Parameters type is invalid");
		static_assert(sizeof...(indexes) == numDims, "Parameters count is invalid");

		return data[getIndex(indexes ...)];
	}

	inline const size_t& size(const int numberDims) const
	{
		assert((numberDims - 1) <= numDims && (numberDims - 1) >= 0 && "Parameters count is invalid");

		return sizes[numberDims - 1];
	}

	inline const vector_size<numDims>& size() const
	{
		return sizes;
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
	template<typename ... Indexes>
	size_t getIndex(Indexes ... indexes)
	{
		impl::checkIndex(sizes.data(), indexes...);

		auto index = impl::index(sizes.data(), dims.data(), indexes...);

		assert(size_t(index) < data.size() && "Parameters count dimensional is invalid");

		return index;
	}

	std::vector<ElementType> data;
	std::array<size_t, numDims> dims;
	std::array<size_t, numDims> sizes;
};


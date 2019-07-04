#pragma once
#include <vector>
#include <type_traits>
#include <cstdlib>
#include <array>
#include <numeric>
#include <cassert> 

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

	template<typename FirstIndex, typename SecondIndex, typename ... Indexes>
	inline static FirstIndex index(size_t *sizes, FirstIndex first, SecondIndex second)
	{
		return first * (*sizes) + second;
	}

	template<typename FirstIndex, typename SecondIndex, typename ... Indexes>
	inline static FirstIndex index(size_t *sizes, FirstIndex first, SecondIndex second, Indexes ... indexes)
	{
		return index(sizes, index(sizes++, first, second), indexes...);
	}

	template<typename FirstIndex, typename ... Indexes>
	inline static FirstIndex index(size_t *sizes, size_t *dims, FirstIndex first)
	{
		return *dims * first;
	}

	template<typename FirstIndex, typename ... Indexes>
	inline static FirstIndex index(size_t *sizes, size_t *dims, FirstIndex first, Indexes ... indexes)
	{
		return *dims * first + index(sizes, dims + 1, indexes...);
	}

	template<typename FirstArg>
	inline void calcCoefficients(/*const*/ size_t *arr, FirstArg first)
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

template<typename ElemetType, size_t numDims>
class NdMatrix
{
public:
	NdMatrix() {};

	template<typename ... Sizes>
	NdMatrix(Sizes ... sizes)
		: data(impl::product(sizes ...))
		, sizes{ size_t(sizes)... }
	{
		static_assert(sizeof...(sizes) == numDims, "Parameters count is invalid");

		assert(impl::isPositive(sizes ...));

		impl::calcCoefficients(dims.data(), sizes ...);
	}

	inline void resize(const std::array<size_t, numDims> &sizesDims)
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
	inline ElemetType& operator()(Indexes ... indexes)
	{
		static_assert(impl::AllNumeric<Indexes...>::value, "Parameters type is invalid");
		static_assert(sizeof...(indexes) == numDims, "Parameters count is invalid");

		return data[getIndex(indexes ...)];
	}

	template<typename ... Indexes>
	inline const ElemetType& operator()(Indexes ... indexes) const
	{
		static_assert(impl::AllNumeric<Indexes...>::value, "Parameters type is invalid");
		static_assert(sizeof...(indexes) == numDims, "Parameters count is invalid");

		return data[getIndex(indexes ...)];
	}

	inline int size(size_t numberDims)
	{
		static_assert((numberDims - 1) < numDims && (numberDims - 1) >= 0, "Parameters count is invalid");

		return sizes[numberDims - 1];
	}

	inline const std::array<size_t, numDims>& size()
	{
		return sizes;
	}

	inline void clear()
	{
		std::vector<ElemetType>().swap(data);
	}

	inline std::vector<ElemetType>& getData()
	{
		return data;
	}

private:
	template<typename ... Indexes>
	size_t getIndex(Indexes ... indexes)
	{
		auto index = impl::index(sizes.data(), dims.data(), indexes...);

		assert(size_t(index) < data.size() && "Parameters count dimensional is invalid");

		return index;
	}

	std::vector<ElemetType> data;
	std::array<size_t, numDims> dims;
	std::array<size_t, numDims> sizes;
};

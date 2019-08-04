// vector_n.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "vector_n.h"
#include <omp.h>
#include <ctime>
#include <cassert>


typedef std::vector<int> vec_int;
typedef std::vector<vec_int> vec2_int;
typedef std::vector<vec2_int> vec3_int;

void testVector4d()
{
	int nx = 200, ny = 60, nz = 70, nt = 200;

	vector_n<int, 4> a(nx, ny, nz, nt);
	std::vector<std::vector<std::vector<vec_int>>> vec4;
	vec4.resize(nx, vec3_int(ny, vec2_int(nz, vec_int(nt))));
	std::vector<int> expected_data;

	size_t start_time_new = clock(); // начальное время

//#pragma omp parallel for
	for (int i = 0, n = 0; i < nx; ++i)
	{
		for (int j = 0; j < ny; ++j)
		{
			for (int k = 0; k < nz; ++k)
			{
				for (int g = 0; g < nt; ++g)
				{
					//expected_data.push_back(n);
					a(i, j, k, g) = i + j + k + g;// n;
					//++n;
				}
			}
		}
	}
	
	size_t search_time = clock() - start_time_new; // искомое в

	std::cout << "TIME 1-DIM VECTOR = " << search_time << std::endl;

	size_t start_time_vec = clock(); // начальное время
//#pragma omp parallel for
	for (int i = 0, n = 0; i < nx; ++i)
	{
		for (int j = 0; j < ny; ++j)
		{
			for (int k = 0; k < nz; ++k)
			{
				for (int g = 0; g < nt; ++g)
				{
					vec4[i][j][k][g] = i + j + k + g;// ++n;
				}
			}
		}
	}

	size_t time_vec = clock() - start_time_vec; // искомое в
	std::cout << "TIME 4-DIM VECTOR = " << time_vec << std::endl;
	std::cout << "TOTAL AMOUNT = " << nx * ny * nz * nt << std::endl;
	std::cout << std::endl;
}

void testVector2d()
{
	int nx = 40000, ny = 4200;

	vector_n<int, 2> a(nx, ny);
	std::vector<vec_int> vec4;
	vec4.resize(nx, vec_int(ny));

	size_t start_time_new = clock(); // начальное время
	for (int i = 0, n = 0; i < nx; ++i)
	{
		for (int j = 0; j < ny; ++j)
		{
			a(i, j) = n;
			++n;
		}
	}

	size_t search_time = clock() - start_time_new; // искомое в

	std::cout << "TIME 1-DIM VECTOR = " << search_time << std::endl;

	size_t start_time_vec = clock(); // начальное время
	for (int i = 0, n = 0; i < nx; ++i)
	{
		for (int j = 0; j < ny; ++j)
		{
			vec4[i][j] = ++n;
		}
	}

	size_t time_vec = clock() - start_time_vec; // искомое в
	std::cout << "TIME 2-DIM VECTOR = " << time_vec << std::endl;
	std::cout << "TOTAL AMOUNT = " << nx * ny << std::endl;
	std::cout << std::endl;

}

void testVector3d()
{
	int nx = 1000, ny = 1000, nz = 160;

	vector_n<int, 3> a(nx, ny, nz);
	std::vector<std::vector<vec_int>> vec3;
	vec3.resize(nx, vec2_int(ny, vec_int(nz)));

	size_t start_time_new = clock(); // начальное время
	for (int i = 0, n = 0; i < nx; ++i)
	{
		for (int j = 0; j < ny; ++j)
		{
			for (int k = 0; k < nz; ++k)
			{
				a(i, j, k) = n;
				++n;
			}
		}
	}

	size_t search_time = clock() - start_time_new;
	std::cout << "TIME 1-DIM VECTOR = " << search_time << std::endl;

	size_t start_time_vec = clock(); // начальное время
	for (int i = 0, n = 0; i < nx; ++i)
	{
		for (int j = 0; j < ny; ++j)
		{
			for (int k = 0; k < nz; ++k)
			{
				vec3[i][j][k] = ++n;
			}
		}
	}

	size_t time_vec = clock() - start_time_vec;
	std::cout << "TIME 3-DIM VECTOR = " << time_vec << std::endl;
	std::cout << "TOTAL AMOUNT = " << nx * ny *nz << std::endl;
	std::cout << std::endl;

}

int main()
{
	testVector4d();
	testVector2d();
	testVector3d();

	return 0;
}
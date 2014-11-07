﻿#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <algorithm>

#include <float.h>

enum quad_type
{
	wall,
	normal,
	convex,
	concave,
	pseudo
};

struct ComputeParameters
{
public:

	double a;
	double b;
	double lb;
	double rb;
	double bb;
	double ub;
	double tau;
	double norm;
	int size;
	int t_count;
	int x_size;
	int y_size;

	ComputeParameters()
	{
		const int x_length_default = 10;
		const int y_length_default = 10;
		t_count = 50;
		tau = 0.02;
		a = 2.;
		// b = 6.486; // при этом значении будут появлятся попадания на стенку для 21 на 21
		b = 10.;
		//  b = 1.;
		lb = bb = 0.;
		rb = ub = 1.;
		double value = pow(2., 0);
		x_size = x_length_default * value;
		y_size = y_length_default * value;
		size = (x_size + 1) * (y_size + 1);
		tau /= value;
		t_count *= value;
	}

	~ComputeParameters()
	{
	}

	int x_length()
	{
		return x_size + 1;
	}

	int y_length()
	{
		return y_size + 1;
	}

	void recompute_params(int step)
	{
		int power = pow(2., step);
		tau /= power;
		t_count *= power;
		x_size *= power;
		y_size *= power;
	}

	int get_size()
	{
		return x_length() * y_length();
	}
};

extern double* compute_density(
	double b,
	double lb,
	double rb,
	double bb,
	double ub,
	double time_step,
	int time_step_count,
	int ox_length,
	int oy_length);

#endif
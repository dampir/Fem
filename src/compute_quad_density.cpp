#include "common.h"
#include "point.h"
#include "utils.h"
#include <algorithm>
#ifdef _OPENMP
  #include <omp.h>
#else
  #define omp_get_thread_num() 1
  #define omp_get_num_threads() 1
#endif

#define sqr(x) ((x)*(x))
#define cub(x) ((x)*(x)*(x))
#define quad(x) ((x)*(x)*(x)*(x))

#ifdef __GNUC__
#define __pure
//#define __pure __attribute__((const))
/*почему то на GCC не работает и ломает результаты расчета*/
#elif __INTEL_COMPILER
#define __pure __declspec(const)
#elif __NVCC__
#define __pure
#else
#define __pure 
#endif

static double B; //-V707
static double UB; //-V707
static double BB; //-V707
static double LB; //-V707
static double RB; //-V707
static double TAU;
static int OX_LEN;
static int OX_LEN_1; // OX_LEN_1
static int OY_LEN;
static int XY_LEN;
static int TIME_STEP_CNT;
static double HX; //-V707
static double HY; //-V707
static double* OX; //-V707
static double* OY; //-V707
static double* PREV_DENSITY;
static double TIME;
static double PREV_TIME; // tau * (tl - 1)

__pure inline static double analytical_solution(double t, double x, double y)
{
	return 1.1 + sin(t * x * y);
}

__pure inline static double func_u(double b, double x, double y)
{
	return b * y * (1 - y) * (M_PI_2 + atan(-x));
}

__pure inline static double func_u(double b, const dp_t& p)
{
	return func_u(b, p.x, p.y);
}

__pure inline static double func_v(double ub, double bb, double lb, double rb, double time, double x, double y)
{
	return atan(0.1 * (x - lb) * (x - rb) * (1 + time) * (y - ub) * (y - bb));
}

__pure inline static double func_v(double ub, double bb, double lb, double rb, double time, const dp_t& p)
{
	return func_v(ub, bb, lb, rb, time, p.x, p.y);
}

__pure inline static double func_f(double b, double time, double ub, double bb, double lb, double rb, double x, double y)
{
	double arg_v = 0.1 * (x - lb) * (x - rb) * (1 + time) * (y - ub) * (y - bb);
	double rho = analytical_solution(time, x, y);
	double drho_dt = x * y * cos(time * x * y);
	double drho_dx = time * y * cos(time * x * y);
	double dtho_dy = time * x * cos(time * x * y);
	double u = func_u(b, x, y);
	double v = func_v(ub, bb, lb, rb, time, x, y);
	double du_dx = -b * y * (1 - y) / (1 + sqr(x));
	double dv_dx = 0.1 * (x - lb) * (x - rb) * (1 + time) * (y - bb + y - ub);
	dv_dx /= (1 + arg_v * arg_v);
	double res = drho_dt + rho * du_dx + u * drho_dx + rho * dv_dx + v * dtho_dy;
	// print_f_params()...
	return res;
}

__pure static void get_coord_on_prev_tl(dp_t& left, dp_t& up, dp_t& right, dp_t& bottom, dp_t& center, int i, int j)
{	
	double u = func_u(B, left);
	double v = func_v(UB, BB, LB, RB, TIME, left);
	left.x = left.x - TAU * u;
	left.y = left.y - TAU * v;
	u = func_u(B, right);
	v = func_v(UB, BB, LB, RB, TIME, right);
	right.x = right.x - TAU * u;
	right.y = right.y - TAU * v;
	u = func_u(B, up);
	v = func_v(UB, BB, LB, RB, TIME, up);
	up.x = up.x - TAU * u;
	up.y = up.y - TAU * v;
	u = func_u(B, bottom);
	v = func_v(UB, BB, LB, RB, TIME, bottom);
	bottom.x = bottom.x - TAU * u;
	bottom.y = bottom.y - TAU * v;
	u = func_u(B, center);
	v = func_v(UB, BB, LB, RB, TIME, center);
	center.x = center.x - TAU * u;
	center.y = center.y - TAU * v;	
}

__pure static double get_det(dp_t& left, dp_t& up, dp_t& right, dp_t& bottom, dp_t& center, int i, int j)
{	
    double w_x_ksi = 0.5*((right.x-center.x)/HX + (center.x - left.x)/HX);
    double w_x_the = 0.5*((up.x-center.x)/HX + (center.x - bottom.x)/HX);
    double w_y_ksi = 0.5*((right.y-center.y)/HY + (center.y - left.y)/HY);
    double w_y_the = 0.5*((up.y-center.y)/HY + (center.y - bottom.y)/HY);

    double r = w_x_ksi*w_y_the - w_x_the *w_y_ksi;
    return r;

}

static double integrate(double prev_density, int i, int j)
{
	dp_t left(OX[i-1], OY[j]);
	dp_t right(OX[i+1], OY[j]);
	dp_t up(OX[i], OY[j+1]);
	dp_t bottom(OX[i], OY[j-1]);
	dp_t center(OX[i], OY[j]);
	get_coord_on_prev_tl(left, up, right, bottom, center, i, j);
	double det = get_det(left, up, right, bottom, center, i, j);		
	return prev_density * det;
}

inline static double get_norm_of_error(double* density, double ts_count_mul_steps)
{
	double r = 0;
	for (int k = 1; k < OY_LEN; ++k)
		for (int j = 1; j < OX_LEN; ++j)
			r += fabs(analytical_solution(ts_count_mul_steps, OX[j], OY[k])
				- density[(OY_LEN + 1) * k + j]);
	return HX * HY * r;
}

static void solve(double* density, double& time)
{
	PREV_DENSITY = new double[XY_LEN];
	for (int j = 0; j < OY_LEN + 1; j++)
	{
		for (int i = 0; i < OX_LEN_1; i++)
		{
			PREV_DENSITY[OX_LEN_1 * j + i] = analytical_solution(0, OX[i], OY[j]);
		}
	}
	
	int i = 0, j = 0, tl = 0;
    double timeStart = 0, timeEnd=0;
#ifdef _OPENMP
    printf("OPENMP THREADS COUNT = %d\n", omp_get_max_threads());
    long count = 0;
    // dummy parallel section to get all threads running
    #pragma omp parallel private(i,j)
    {
      _InterlockedIncrement(&count);
    }
#endif
 
#ifdef _OPENMP
    printf("OPENMP timer function is used!\n");
    timeStart = omp_get_wtime();
#else
    printf("Standart timer function is used!\n");
    StartTimer();
#endif        
    fflush(stdout);
	for (tl = 1; tl <= TIME_STEP_CNT; tl++)
	{	    	
		PREV_TIME = TIME;
		TIME = TAU * tl;
		for (int k = 0; k <= OX_LEN; k++)
		{
			density[k] = analytical_solution(OX[k], BB, TIME);
			density[OX_LEN_1 * OY_LEN + k] = analytical_solution(OX[k], UB, TIME);
		}

		for (int u = 0; u <= OY_LEN; u++)
		{
			density[OX_LEN_1 * u] = analytical_solution(LB, OY[u], TIME);
			density[OX_LEN_1 * u + OX_LEN] = analytical_solution(RB, OY[u], TIME);
		}
#ifdef _OPENMP
   #pragma omp parallel for collapse(2) private(i, j)
#endif
		for (j = 1; j < OY_LEN; ++j)
		{
			for (i = 1; i < OX_LEN; ++i)
			{				
				density[OX_LEN_1 * j + i] = integrate(PREV_DENSITY[OX_LEN_1 * j + i], i, j);
				density[OX_LEN_1 * j + i] += TAU * func_f(B, TIME, UB, BB, LB, RB, OX[i], OY[j]);
			}
		}
		memcpy(PREV_DENSITY, density, XY_LEN * sizeof(double));// заменить на быструю версию из agnerasmlib
	}
#ifdef _OPENMP
	timeEnd = omp_get_wtime();
	time = timeEnd-timeStart;
	printf("time %f s.\n", time);
#else
	time = GetTimer();
	printf("time %f s.\n", time/1000);
#endif
	delete [] PREV_DENSITY;
}

inline static void init(double b, double lb, double rb, double bb, double ub,
                        double tau, int time_step_count, int ox_length, int oy_length)
{
	B = b;
	UB = ub;
	BB = bb;
	LB = lb;
	RB = rb;
	TAU = tau;
	TIME_STEP_CNT = time_step_count;
	XY_LEN = (ox_length + 1) * (oy_length + 1);
	OX_LEN = ox_length;
	OX_LEN_1 = ox_length + 1;
	OY_LEN = oy_length;
	OX = new double[OX_LEN_1];
	OY = new double[OY_LEN + 1];
	for (int i = 0; i <= OX_LEN; ++i) OX[i] = lb + i * (rb - lb) / OX_LEN;
	for (int i = 0; i <= OY_LEN; ++i) OY[i] = bb + i * (ub - bb) / OY_LEN;
	HX = OX[1] - OX[0];
	HY = OY[1] - OY[0];
}

inline static void clean()
{
	B = 0;
	UB = 0;
	BB = 0;
	LB = 0;
	RB = 0;
	TAU = 0;
	TIME = 0;
	OX_LEN = 0;
	OY_LEN = 0;
	OX_LEN_1 = 0;
	TIME_STEP_CNT = 0;
	XY_LEN = 0;
	HX = 0;
	HY = 0;
	delete [] OX;
	delete [] OY;
}

double* compute_quad_density(double b, double lb, double rb, double bb, double ub,
                        double tau, int time_step_count, int ox_length, int oy_length, double& norm, double& time)
{
	init(b, lb, rb, bb, ub, tau, time_step_count, ox_length, oy_length);
	double* density = new double[XY_LEN];
	print_params(B, LB, RB, BB, UB, TAU, TIME_STEP_CNT, OX_LEN, OY_LEN);
	solve(density, time);
	norm = get_norm_of_error(density, TIME_STEP_CNT * TAU);
	clean();
	return density;
}
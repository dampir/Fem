#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include "common.h"
#include "test_utils.h"
#include "LowOrdOper.h"
#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
#ifdef _OPENMP
  #include <omp.h>
#else
  #define omp_get_thread_num() 1
  #define omp_get_num_threads() 0
  #define OMP_NUM_THREADS -1
#endif

class cpu : public testing::Test
{
public:
	cpu()
	{
	    #ifdef VER
//		 printf("%s\n", STRINGIZE_VALUE_OF(VER));
	    #endif
	}
	
protected:
	double* solve_quad_internal(ComputeParameters& p, double& time)
	{
		return compute_quad_density(p.b, p.lb, p.rb, p.bb,
		                       p.ub, p.tau, p.t_count, p.x_size,
		                       p.y_size, p.norm, time);
	}

	double* solve_quad2_internal(ComputeParameters& p, double& time)
	{
		return compute_quad2_density(p.b, p.lb, p.rb, p.bb,
		                       p.ub, p.tau, p.t_count, p.x_size,
		                       p.y_size, p.norm, time);
	}

	double* solve_internal(ComputeParameters& p, double& time)
	{
		return compute_density(p.b, p.lb, p.rb, p.bb,
		                       p.ub, p.tau, p.t_count, p.x_size,
		                       p.y_size, p.norm, time);
	}
	
	double* solve_internal_cuda(ComputeParameters& p, float& time)
	{
		return compute_density_cuda(p.b, p.lb, p.rb, p.bb, p.ub, p.tau, p.t_count, p.x_size, p.y_size, p.norm, time);
	}

	double* solve_internal_quad_cuda(ComputeParameters& p, float& time)
	{
		return compute_density_quad_cuda(p.b, p.lb, p.rb, p.bb, p.ub, p.tau, p.t_count, p.x_size, p.y_size, p.norm, time);
	}
	

	double* get_model_result(ComputeParameters& p, int lvl)
	{
		return solByEqualVolWithVarStepPlusPrint1(p.a, p.b, p.lb, p.rb, p.bb,
		                                          p.ub, p.tau, p.t_count, p.x_size,
		                                          p.y_size, lvl);
	}

	void print_result_table_header()
	{
		fflush(stdout);
		printf("|===============================================================================================|\n");	
		printf("|ALGO\t\t|\tSIZE\t|\tTIME\t\t|\tNORM\t\t|\tOMP\t|\n");
		printf("|===============================================================================================|\n");	
	}

	void print_result_table_footer()
	{
		printf("|===============================================================================================|\n");	
	}

	void print_result_table_row(std::string algo_name, int size, float time, double norm)
	{
		fflush(stdout);
		int omptthreads = strTo<int>(GetEnv("OMP_NUM_THREADS"));
		std::string ompth = omptthreads <= 0 ? "NO OMP" : toStr<int>(omptthreads);
		ompth = omptthreads >= 64 ? "NO OMP" : toStr<int>(omptthreads);
		printf("|%s\t|\t%d\t|\t%le\t|\t%le\t|\t%s\t|\n", algo_name.c_str(), size, time, norm, ompth.c_str());
		fflush(stdout);
	}
};

//TEST_F(cpu, DISABLED_test_to_model)
TEST_F(cpu, test_to_model)
{
	int first = 0, last = 3;
	double norm_test, norm_model;
	double time;
	ComputeParameters p = ComputeParameters();
	for (int lvl = first; lvl < last; ++lvl)
	{
		p.recompute_params(lvl);
		double* data = solve_internal(p, time);
		double norm_test = p.norm;
		double* model = get_model_result(p, lvl);
		double norm_model = p.norm;
		if (lvl < 2)
			for (int i = 0; i < p.get_size(); i++)
			{
				ASSERT_NEAR(model[i], data[i], 1e-12);
			}
		printf("model norm = %f\n", norm_model);
		printf("test norm = %f\n", norm_test);
		printf("Time: %lf\n", time);
		ASSERT_NEAR(norm_model, norm_test, 1e-12);
		delete[] data;
		delete[] model;
	}
	
}

TEST_F(cpu, test_to_model_cuda)
{
	int first = 0, last = 1;
	double norm_test, norm_model;
	float time = 0;
	ComputeParameters p = ComputeParameters();
	for (int lvl = first; lvl < last; ++lvl)
	{
		p.recompute_params(lvl);
		double* data = solve_internal_cuda(p, time);
		double norm_test = p.norm;
		double* model = get_model_result(p, lvl);
		printf("GPU\n");
		_print_matrix(data, 11, 11);
		printf("CPU\n");
		_print_matrix(model, 11, 11);

		double norm_model = p.norm;
//		if (lvl < 2)
			for (int i = 0; i < p.get_size(); i++)
			{
				ASSERT_NEAR(model[i], data[i], 1e-12);
			}
		printf("model norm = %f\n", norm_model);
		printf("test norm = %f\n", norm_test);
		ASSERT_NEAR(norm_model, norm_test, 1e-12);
		delete[] data;
		delete[] model;
	}
}

TEST_F(cpu, test_fma)
{
	int first = 3, last = 4;
	double norm_test, norm_model;
	float time = 0;
	ComputeParameters p = ComputeParameters();
	for (int lvl = first; lvl < last; ++lvl)
	{
		p.recompute_params(lvl);
		double* data = solve_internal_cuda(p, time);
		double norm_test = p.norm;
		double* model = get_model_result(p, lvl);	
		double norm_model = p.norm;
		for (int i = 0; i < p.get_size(); i++)
		{
//			ASSERT_NEAR(model[i], data[i], 1e-9);
		//	ASSERT_NEAR(model[i], data[i], 1e-12);
		}
		printf("model norm = %.8f\n", norm_model);
		printf("test norm = %.8f\n", norm_test);
		ASSERT_NEAR(norm_model, norm_test, 1e-12);
		delete[] data;
		delete[] model;
	}
}

TEST_F(cpu, china2015_test)
{
	int first = 4, last = 9;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	for (int lvl = first; lvl < last; ++lvl)
	{
		printf("Start level %d\n", lvl);
		fflush(stdout);
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_internal(p, time);
		delete[] data;
	}
}

TEST_F(cpu, valgrind_test)
{
	int first = 1, last = 2;
	double time = 0;

	ComputeParameters p = ComputeParameters();
	for (int lvl = first; lvl < last; ++lvl)
	{
		fflush(stdout);
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_internal(p, time);
		delete[] data;
	}
}

// test new version with replace of variables of integral
TEST_F(cpu, quad_test)
{
	int first = 0, last = 4;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	for (int lvl = first; lvl < last; ++lvl)
	{		
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_quad_internal(p, time);
		print_result_table_row("cpu_quad", p.x_length(), time, p.norm);		 
		delete[] data;		
	}
	for (int lvl = first; lvl < last; ++lvl)
	{
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_internal(p, time);		
		print_result_table_row("cpu_orig", p.x_length(), time, p.norm);
		delete[] data;
	}
	print_result_table_footer();
}

// test new version with replace of variables of integral
// with bilinear isoparameter transformation
// compare with previous quad
TEST_F(cpu, quad2_test)
{
	int first = 0, last = 1;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	for (int lvl = first; lvl < last; ++lvl)
	{
		time = 0;
		p.recompute_params(lvl);
		p.t_count = 1;
		double* data = solve_quad2_internal(p, time);		
		print_result_table_row("cpu_quad2", p.x_length(), time, p.norm);
		delete[] data;
	}
	print_result_table_footer();
}

// ТЕСТ С НАЛЕТОМ ТОЧЕК НА СТЕНКУ ДЛЯ АЛГОРИТМА QUAD
TEST_F(cpu, quad_wall_collision_test)
{
	int first = 0, last = 4;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	for (int lvl = first; lvl < last; ++lvl)
	{		
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_quad_internal(p, time);
		print_result_table_row("cpu_quad", p.x_length(), time, p.norm);		 
		delete[] data;		
	}

	for (int lvl = first; lvl < last; ++lvl)
	{		
		time = 0;
		p.recompute_params(lvl, 2); // ДЕЛЕАЕМ СЕТКУ В 2 РАЗА МЕЛЬЧЕ, ЧТОБЫ БЫЛИ СТОЛКНОВЕНИЯ СО СТЕНКОЙ
		double* data = solve_quad_internal(p, time);
		print_result_table_row("cpu_quad_wc", p.x_length(), time, p.norm);		 
		delete[] data;		
	}
	
	print_result_table_footer();
}

TEST_F(cpu, cuda_quad_2561_test)
{
	int first = 8, last = 9;
	float time_cuda = 0;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	for (int lvl = first; lvl < last; ++lvl)
	{		
		fflush(stdout);
		time = 0;
		p.recompute_params(lvl);		
		double* data = solve_internal_quad_cuda(p, time_cuda);
		print_result_table_row("gpu_quad", p.x_length(), time_cuda, p.norm);		
		delete[] data;
	}
	print_result_table_footer();
}

// test new version with replace of variables of integral
TEST_F(cpu, cuda_quad_test)
{
	int first = 0, last = 1;
	float time_cuda = 0;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	for (int lvl = first; lvl < last; ++lvl)
	{
		fflush(stdout);
		time = 0;
		p.recompute_params(lvl);
		//p.t_count = 1;

		double* data = solve_quad_internal(p, time);
		print_result_table_row("cpu_quad", p.x_length(), time, p.norm);
		//_print_matrix(data, p.x_size+1, p.y_size+1);
		delete[] data;

		data = solve_internal_quad_cuda(p, time_cuda);
		print_result_table_row("gpu_quad", p.x_length(), time_cuda, p.norm);
		//_print_matrix(data, p.x_size+1, p.y_size+1);
		delete[] data;
	}
	print_result_table_footer();
}


/* ГРУППА ТЕСТОВ ДЛЯ ОБЩЕГО ТЕСТИРОВАНИЯ  СТАРЫХ И НОВЫХ АЛГООИТМОВ ДЛЯ СТАТЬИ 2015 ГОДА */

// ДИАПОЗОН СЕТОК ОТ 11 ДО 2561 (УРОВНИ: 0 ДО 8 ВКЛЮЧИТЕЛЬНО)

// 1) РЕЗУЛЬТАТ: ТАБЛИЦА НОРМ ДЛЯ СЕТОК ОТ 11 ДО 2561 ДЛЯ СТАРОГО И НОВОГО ПОСЛ. АЛГОРИТМОВ
TEST_F(cpu, cpu_2_norm_test) 
{
	int first = 0, last = 9;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	fflush(stdout);
	for (int lvl = first; lvl < last; ++lvl)
	{
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_internal(p, time); delete[] data;
		print_result_table_row("cpu_orig", p.x_length(), time, p.norm);
 	}
	for (int lvl = first; lvl < last; ++lvl)
	{
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_quad_internal(p, time); delete[] data;
		print_result_table_row("cpu_quad", p.x_length(), time, p.norm);
	}
	print_result_table_footer();
}

// 2) ЗАМЕРЫ ВРЕМЕНИ РАСЧЕТОВ CUDA НОВОГО И СТАРОГО АЛГОРИТМОВ
//	  РЕЗУЛЬТАТ: ТАБЛИЦА С ВРЕМЕНЕМ СЧЕТА СТАРЫМ И НОВЫМ CUDA АЛГОРИТМОМ
TEST_F(cpu, gpu_2_time_test)
{
	int first = 0, last = 9;
	float time_cuda = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	for (int lvl = first; lvl < last; ++lvl) 
	{
		time_cuda = 0;
		p.recompute_params(lvl);
		double* data = solve_internal_quad_cuda(p, time_cuda); delete[] data;
		print_result_table_row("gpu_quad", p.x_length(), time_cuda, p.norm);
	}
	for (int lvl = first; lvl < last; ++lvl) 
	{
		time_cuda = 0;
		p.recompute_params(lvl);
		double* data = solve_internal_cuda(p, time_cuda); delete[] data;
		print_result_table_row("gpu_old", p.x_length(), time_cuda, p.norm);
	}	
	print_result_table_footer();
}

// 2.1) ЗАМЕРЫ ВРЕМЕНИ РАСЧЕТОВ CUDA НОВОГО АЛГОРИТМОВ
//	  РЕЗУЛЬТАТ: ТАБЛИЦА С ВРЕМЕНЕМ СЧЕТА НОВЫМ CUDA АЛГОРИТМОМ
TEST_F(cpu, gpu_2_time_test_1)
{
	int first = 6, last = 8;
	float time_cuda = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	for (int lvl = first; lvl < last; ++lvl) 
	{
		time_cuda = 0;
		p.recompute_params(lvl);
		double* data = solve_internal_quad_cuda(p, time_cuda); delete[] data;
		print_result_table_row("gpu_quad", p.x_length(), time_cuda, p.norm);
	}
	print_result_table_footer();
}

// 3) ЗАМЕРЫ ВРЕМЕНИ РАСЧЕТОВ OPENMP НОВОГО И СТАРОГО АЛГОРИТМОВ
//	  РЕЗУЛЬТАТ: ТАБЛИЦА С ВРЕМЕНЕМ СЧЕТА СТАРЫМ И НОВЫМ OPENMP АЛГОРИТМОМ
TEST_F(cpu, omp_2_time_test) 
{
	int first = 0, last = 9;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	fflush(stdout);
	for (int lvl = first; lvl < last; ++lvl)
	{		
		time = 0;
		p.recompute_params(lvl);		
		double* data = solve_internal(p, time); delete[] data;
		print_result_table_row("omp_cpu_orig", p.x_length(), time, p.norm);		
	}
	for (int lvl = first; lvl < last; ++lvl)
	{		
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_quad_internal(p, time); delete[] data;
		print_result_table_row("omp_cpu_quad", p.x_length(), time, p.norm);
	}
	print_result_table_footer();
}

// 3.1) ЗАМЕРЫ ВРЕМЕНИ РАСЧЕТОВ OPENMP СТАРОГО АЛГОРИТМА ДЛЯ 641 РЕТЕСТ
TEST_F(cpu, omp_2_time_test_16_641) 
{
	int first = 6, last = 7;
	double time = 0;
	ComputeParameters p = ComputeParameters();
	print_result_table_header();
	fflush(stdout);
	for (int lvl = first; lvl < last; ++lvl)
	{
		time = 0;
		p.recompute_params(lvl);
		double* data = solve_internal(p, time); delete[] data;
		print_result_table_row("omp_cpu_orig", p.x_length(), time, p.norm);
	}
	print_result_table_footer();
}

/* ===================================================================================== */

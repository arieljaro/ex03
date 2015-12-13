//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
//depending files: 

//--------Library Includs--------//
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <WinBase.h>
#include <tchar.h>

//--------Project Includs--------//
#include "Worker.h"
#include "SimpleWinAPI.h"

//--------Definations--------//
#define INPUT_PARAMETERS_NUM (9)
typedef enum {
	CMD_PARAMETER_NUM_OF_WORKERS_OFFSET = 1,
	CMD_PARAMETER_N_OFFSET,
	CMD_PARAMETER_JOB_SIZE_OFFSET,
	CMD_PARAMETER_SUB_SEQ_LENGTH_OFFSET,
	CMD_PARAMETER_FAILURE_PERIOD_OFFSET,
	CMD_PARAMETER_A1_OFFSET,
	CMD_PARAMETER_D_OFFSET,
	CMD_PARAMETER_Q_OFFSET,
} CmdParameter;

//--------Global Variables---------//
HANDLE work_semaphore;

//--------Function Declarations--------//
/**
* This function creates a thread by calling Win32 Api's CreateThread()
* function, and setting some of the parameters to default value.
*
* parameters:
* ----------
* StartAddress - a pointer to the function that will be run by the thread.
* ParameterPtr - a pointer to the parameter that will be supplied to the
*                function run by the thread.
* ThreadIdPtr - return argument: a pointer to a DWORD variable into which
*               the function will write the created thread's ID.
*
* returns:
* --------
* On success, a handle to the created thread. On Failure - NULL.
*/
HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE StartAddress,LPVOID ParameterPtr,LPDWORD ThreadIdPtr);
//This function Initialize series type, 
//initilize the fields inside: Including creating mutex, and fill job_size and jobs_num
//initilize the arrays inside the series type (Jobs array)
//The function gets a pointer to series type and fill its field, including malloc fo required arrays
BOOL IntializeSeries(Series *series, int job_size, int jobs_num);
/* Reads the parametes and sets their values in the corresponding parameters */
BOOL HandleParameters(
	int argc,
	char *argv[],
	int *num_of_workers,
	int *N,
	int *job_size,
	int *sub_seq_length,
//	int *failure_period,
	int *a1,
	int *d,
	int *q
);
//--------Implementation--------//
//--------Main--------//
int main(int argc, char *argv[])
{
	ErrorCode error_code = SUCCESS;
	int num_of_workers;
	int N;
	int job_size;
	int sub_seq_length;
	//int failure_period;
	int a1;
	int d;
	int q;
	int jobs_num;
	Series arithmetic_series;
	//Series geometric_series;
	//Series diffrential_between_arith_geom;
	int i;
	HANDLE *threads_handles = NULL; //an array to hold the handles, the size isn't known during compilation time
	DWORD *threads_id = NULL;
	//DWORD exitcode;
	//DWORD wait_code;

	//checking parameters and print
	if (!HandleParameters(
		argc,
		argv,
		&num_of_workers,
		&N,
		&job_size,
		&sub_seq_length,
//		&failure_period,
		&a1,
		&d,
		&q
	))
	{
		LOG_ERROR("Failed to parse the cmd parameters");
		error_code = WRONG_PARAMETERS;
		goto cleanup;
	}
	LOG_INFO("Started Building 3 series with the following parameters: \r\n \
			 Num Of Workers = %d \r\n \
			 N = %d \r\n \
			 Job Size = %d \r\n \
			 Sub Sequence Length = %d \r\n \
			 A1 = %d \r\n \
			 d = %d \r\n \
			 q = %d",
		num_of_workers,
		N,
		job_size,
		sub_seq_length,
		a1,
		d,
		q
	);
	jobs_num= sub_seq_length/job_size;
	if(!IntializeSeries(&arithmetic_series, job_size, jobs_num))
	{
		LOG_ERROR("Failed to intilize the arithmatic series");
		error_code = INTIALIZE_SERIES_FAILED;
		goto cleanup;
	}

		//creating an array of handles in num_of_workerss size
	threads_handles = (HANDLE *)malloc(num_of_workers * sizeof(HANDLE));
	if (threads_handles == NULL)
	{
		LOG_ERROR("failed to malloc memory for threads_handle array");
		error_code = MALLOC_FAILED;
		goto cleanup;
	}
	
	//allocate memory for thread ID
	threads_id = (DWORD *)malloc(num_of_workers * sizeof(DWORD));
	if (threads_id == NULL)
	{
		LOG_ERROR("failed to malloc memory for threads_id array");
		error_code = MALLOC_FAILED;
		goto cleanup;
	}

	//Creating for each test, thread to run the test.
	for (i=0; i < num_of_workers; i++)
	{
		threads_handles[i] = CreateThreadSimple(
			(LPTHREAD_START_ROUTINE)RunThread,
			&arithmetic_series,
			(LPDWORD)&(threads_id[i])
		);
		
		//cheack the creation of thread succeedded
		if (threads_handles[i] == NULL)
		{
			LOG_ERROR("failed to create thread");
			error_code = THREAD_CREATION_FAILED;
			goto cleanup;
		}

		LOG_INFO("Created thread number %d with id %d", i, threads_id[i]);
	}


	cleanup:
return error_code;
}

//--------Functions used in main--------//

/* Reads the parametes and sets their values in the corresponding parameters */
BOOL HandleParameters(
	int argc,
	char *argv[],
	int *num_of_workers,
	int *N,
	int *job_size,
	int *sub_seq_length,
//	int *failure_period,
	int *a1,
	int *d,
	int *q
){
	int atoi_result;
//check for validity of number of arguments
	if (argc < INPUT_PARAMETERS_NUM)
	{
		LOG_ERROR("too few arguments were send to building series process, exiting");
		return FALSE;
	}
	
	if (argc > INPUT_PARAMETERS_NUM)
	{
		LOG_ERROR("too many arguments were send to building series process, exiting");
		return FALSE;
	}

	//Convert all strings to Integer and assign them to the right parameter.

	atoi_result = atoi(argv[CMD_PARAMETER_NUM_OF_WORKERS_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result < 0))
	{
		LOG_ERROR("Wrong num of workers parameter");
		return FALSE;
	}
	*num_of_workers= atoi_result;

	atoi_result = atoi(argv[CMD_PARAMETER_N_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result < 0))
	{
		LOG_ERROR("Wrong N parameter- number of indexes to compute ");
		return FALSE;
	}
	*N= atoi_result;
	atoi_result = atoi(argv[CMD_PARAMETER_JOB_SIZE_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result < 0))
	{
		LOG_ERROR("Wrong job size parameter");
		return FALSE;
	}
	*job_size= atoi_result;
	atoi_result = atoi(argv[CMD_PARAMETER_SUB_SEQ_LENGTH_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result < 0))
	{
		LOG_ERROR("Wrong sub sequence length parameter");
		return FALSE;
	}
	*sub_seq_length= atoi_result;
	atoi_result = atoi(argv[CMD_PARAMETER_A1_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result < 0))
	{
		LOG_ERROR("Wrong First term in the series - a1 parameter");
		return FALSE;
	}
	*a1= atoi_result;
	atoi_result = atoi(argv[CMD_PARAMETER_D_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result < 0))
	{
		LOG_ERROR("Wrong common difference in the arithmetic series - d parameter");
		return FALSE;
	}
	*d= atoi_result;
	atoi_result = atoi(argv[CMD_PARAMETER_Q_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result < 0))
	{
		LOG_ERROR("Wrong factor between the terms in the geometric series- q parameter");
		return FALSE;
	}
	*q= atoi_result;
	return TRUE;
}

BOOL IntializeSeries(Series *series, int job_size, int jobs_num)
{
	series->job_size=job_size;
	series->sub_num_jobs=sub_num_jobs;
	return TRUE;
}
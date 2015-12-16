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

//--------Definitions--------//
typedef enum {
	CMD_PARAMETER_NUM_OF_WORKERS_OFFSET = 1,
	CMD_PARAMETER_N_OFFSET,
	CMD_PARAMETER_JOB_SIZE_OFFSET,
	CMD_PARAMETER_SUB_SEQ_LENGTH_OFFSET,
	CMD_PARAMETER_FAILURE_PERIOD_OFFSET,
	CMD_PARAMETER_A1_OFFSET,
	CMD_PARAMETER_D_OFFSET,
	CMD_PARAMETER_Q_OFFSET,
	CMD_PARAMETERS_NUM
} CmdParameter;

//--------Global Variables---------//
volatile HANDLE work_semaphore = NULL;

//--------Declarations--------//
//The function gets a pointer to series typeand pwrameters of all series types: job_size, jobs num, a1,d, q and the type of the series
//use the series input for the output:
//initilize the fields inside: Including creating mutex, and fill job_size, jobs_num ...
//initilize the arrays inside the series type (Jobs array)- performing required malloc 
BOOL IntializeSeries(Series *series, int job_size, int jobs_num, float a1, float d, float q, int N, unsigned int semaphore_size, SeriesType type);

//internal function in ItializeSeries, responsible for intilize the jobs array
BOOL InitilizeJobsArray (Series *series, int job_size, int jobs_num);

/* Reads the parametes and sets their values in the corresponding parameters */
BOOL HandleParameters(
	int argc,
	char *argv[],
	int *num_of_workers,
	int *N,
	int *job_size,
	int *sub_seq_length,
//	int *failure_period,
	float *a1,
	float *d,
	float *q
);

//$// debug function - to delete
BOOL RunThreadTest(Series *series, int jobs_num);

//--------Implementation--------//
//--------Main--------//
int main(int argc, char *argv[])
{
	ErrorCode error_code = GENERAL_FAILURE;
	int num_of_workers;
	int N;
	int job_size;
	int sub_seq_length;
	//int failure_period;
	float a1;
	float d;
	float q;
	int jobs_num;
	// TODO: should be volatile??
	Series arithmetic_series;
	//Series geometric_series;
	//Series diffrential_between_arith_geom;
	int i;
	unsigned int semaphore_size;
	HANDLE *threads_handles = NULL; //an array to hold the handles, the size isn't known during compilation time
	DWORD *threads_id = NULL;
	DWORD exitcode;
	DWORD wait_code;
	BOOL were_series_initialized = FALSE;
	DWORD start_tick = 0;
	DWORD end_tick = 0;

	start_tick = GetTickCount();

	//----checking parameters and print---//
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
			 A1 = %f \r\n \
			 d = %f \r\n \
			 q = %f",
		num_of_workers,
		N,
		job_size,
		sub_seq_length,
		a1,
		d,
		q
	);
	jobs_num = sub_seq_length / job_size;

	//----intilize the semaphore---// 
	semaphore_size = SERIES_TYPES_COUNT * jobs_num;
	work_semaphore = CreateSemaphore( 
		NULL,				// Default security attributes 
		semaphore_size,		// Initial Count - the number of open "jobs" for workers in the start
		semaphore_size,		// Maximum Count -equal to the intial value
		NULL); // un-named 
	if (work_semaphore == NULL)
	{
		LOG_ERROR("Failed to create semaphore for global num of workers");
		error_code = CREATE_SEMAPHORE_FAILED;
		goto cleanup;
	}
	
	//----intilize The series structure---//
	if(!IntializeSeries(&arithmetic_series, job_size, jobs_num, a1, d, q, N, semaphore_size, ARITHMETIC_SERIES))
	{
		LOG_ERROR("Failed to intilize the arithmatic series");
		error_code = INTIALIZE_SERIES_FAILED;
		goto cleanup;
	}
	were_series_initialized = TRUE;

	//----Starting threads workers---//
	//creating an array of handles in num_of_workers size
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

	//Creating the threads, which are the "worker" threads
	for (i=0; i < num_of_workers; i++)
	{
		threads_handles[i] = CreateThreadSimple(
			(LPTHREAD_START_ROUTINE)RunThread,
			&arithmetic_series,
			(LPDWORD)&(threads_id[i])
		);
		
		//check if the creation of thread succeedded
		if (threads_handles[i] == NULL)
		{
			LOG_ERROR("failed to create thread");
			error_code = THREAD_CREATION_FAILED;
			goto cleanup;
		}

		LOG_INFO("Created thread number %d with id %d", i, threads_id[i]);
	}
	
	//Wait for all threads to end
	wait_code = WaitForMultipleObjects(
		num_of_workers,
		threads_handles,
		TRUE,       // wait until all threads finish 
		INFINITE
	);

	//----All threads workers finished their jobs, get errors---//
	if (wait_code != WAIT_OBJECT_0)
	{
		LOG_ERROR("Unexpected output value of 0x%x from WaitForMultipleObject()", wait_code);
		error_code = WAIT_FOR_MULTIPLE_OBJECT_FAILED;
		goto cleanup;
	}
	
	//get exit code of each thread and close the handle
	for (i = 0; i < num_of_workers; i++)
	{
		if (!GetExitCodeThread(threads_handles[i], &exitcode))
		{
			LOG_ERROR("Failed to get thread #%d exit code", i);
			error_code = THREAD_RUN_FAILED;
			goto cleanup;
		}
		LOG_INFO("Thread number %d returned with exit code %d (1=success)", i, exitcode);
		if(exitcode != TRUE)
		{
			LOG_ERROR("Thread number %d failed", i);
			error_code = THREAD_RUN_FAILED;
			goto cleanup;
		}
		
		CloseHandle(threads_handles[i]);
		threads_handles[i] = NULL;
	}

	//Clean(&arithmetic_series);

	// If we reach this point then we set the error_code to success
	error_code = SUCCESS;
	
	//----Clean up section: free all memory and wxit with correct exit code---//
cleanup:
	if (threads_handles != NULL)
	{
		for (i = 0; i < num_of_workers; i++)
		{
			if (threads_handles[i] != NULL)
			{
				CloseHandle(threads_handles[i]);
			}
		}
		free(threads_handles);
	}

	if (threads_id != NULL)
	{
		free(threads_id);
	}

	if (work_semaphore != NULL)
	{
		CloseHandle(work_semaphore);
	}

	/* CR: implement a function destroy_series with all this logic */
	//for...
	if (were_series_initialized)
	{
		if ((arithmetic_series.mutex_cleaning) != NULL)
		{
			CloseHandle(arithmetic_series.mutex_cleaning);
		}
		
		if ((arithmetic_series.mutex_building) != NULL)
		{
			CloseHandle(arithmetic_series.mutex_building);
		}
		
		if ((arithmetic_series.jobs_array) != NULL)
		{
			for (i = 0; i < jobs_num; i++)
			{
				free (arithmetic_series.jobs_array[i].values_arr);
			}
			free (arithmetic_series.jobs_array);
		}

		if (arithmetic_series.output_file != NULL)
		{
			fclose(arithmetic_series.output_file);
		}
	}

	end_tick = GetTickCount();
	LOG_INFO("Program End: 3 Series builder exited with exit code %d (total running time = %f)", error_code, (end_tick - start_tick) / 1000.0);
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
	float *a1,
	float *d,
	float *q
){
	int atoi_result;
	double atof_result;

//check for validity of number of arguments
	if (argc < CMD_PARAMETERS_NUM)
	{
		LOG_ERROR("too few arguments were send to building series process, exiting");
		return FALSE;
	}
	
	if (argc > CMD_PARAMETERS_NUM)
	{
		LOG_ERROR("too many arguments were send to building series process, exiting");
		return FALSE;
	}

	//Convert all strings to Integer and assign them to the right parameter.
	atoi_result = atoi(argv[CMD_PARAMETER_NUM_OF_WORKERS_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result <= 0))
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
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result <= 0))
	{
		LOG_ERROR("Wrong job size parameter");
		return FALSE;
	}
	*job_size= atoi_result;

	atoi_result = atoi(argv[CMD_PARAMETER_SUB_SEQ_LENGTH_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result <= 0))
	{
		LOG_ERROR("Wrong sub sequence length parameter");
		return FALSE;
	}
	*sub_seq_length= atoi_result;

	atof_result = atof(argv[CMD_PARAMETER_A1_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL))
	{
		LOG_ERROR("Wrong First term in the series - a1 parameter");
		return FALSE;
	}
	*a1 = (float)atof_result;

	atof_result = atof(argv[CMD_PARAMETER_D_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL))
	{
		LOG_ERROR("Wrong common difference in the ARITHMETIC_SERIES series - d parameter");
		return FALSE;
	}
	*d = (float)atof_result;

	atof_result = atof(argv[CMD_PARAMETER_Q_OFFSET]);
	if ((errno == ERANGE) || (errno == EINVAL))
	{
		LOG_ERROR("Wrong factor between the terms in the GEOMETRIC_SERIES series- q parameter");
		return FALSE;
	}
	*q = (float)atof_result;

	return TRUE;
}

BOOL IntializeSeries(Series *series, int job_size, int jobs_num, float a1, float d, float q, int N, unsigned int semaphore_size, SeriesType type)
{
	HANDLE mutex_cleaning = NULL;
	HANDLE mutex_building = NULL;
	LPCTSTR mutex_name_clean = _T("MutexClean");
	LPCTSTR mutex_name_build = _T("MutexBuild");
	const char *output_filename;
	errno_t err = -1;
	FILE *output_file = NULL;
	
	if (series == NULL)
	{
		LOG_ERROR("received NULL pointer for intilize series");
		return FALSE;
	}
	
	series->type			  = type;
	series->job_size		  = job_size;
	series->jobs_num		  = jobs_num;
	series->a1				  = a1;
	series->d				  = d;
	series->q				  = q;
	series->N				  = N;
	series->semaphore_size	  = semaphore_size;	
	series->next_job_to_build = 0;
	series->next_job_to_clean = 0;
	series->cleaning_state    = NOTHING_TO_CLEAN;
	series->output_file		  = NULL;
	
	// open the series output file
	switch (series->type)
	{
		case ARITHMETIC_SERIES:
			output_filename = ARITHMETIC_OUTPUT_FILENAME;
			break;
		default:
			LOG_ERROR("Found an invalid series type: %d", series->type);
	}
	err = fopen_s(&output_file, output_filename, "w");
    if (err != 0)
    {
        LOG_ERROR("couldn't create output file for series %d: couldn't open the file", series->type);
        return FALSE;
    }
	series->output_file = output_file;
	
	// Initialize Jobs Array
	if (!InitilizeJobsArray(series, job_size, jobs_num))
	{
		LOG_ERROR("failed to initilize Jobs Array for the series");
		return FALSE;
	}

	//Creating mutex// 
	mutex_cleaning = CreateMutex( 
		NULL,   // default security attributes 
		FALSE,	// don't lock mutex immediately 
		mutex_name_clean); //"MutexClean"
	if ( mutex_cleaning == NULL )
	{
		LOG_ERROR("failed to create mutex, returned with error %d",GetLastError());
		return FALSE;
	}
	series->mutex_cleaning = mutex_cleaning;

	mutex_building = CreateMutex(
		NULL,   // default security attributes 
		FALSE,	// don't lock mutex immediately 
		mutex_name_build); //"MutexBuild"
	if ( mutex_building == NULL )
	{
		LOG_ERROR("failed to create mutex, returned with error %d",GetLastError());
		return FALSE;
	}
	series->mutex_building = mutex_building;

	return TRUE;
}

BOOL InitilizeJobsArray (Series *series, int job_size, int jobs_num)
{
	int i=0;
	series->jobs_array = (JobArray)malloc(jobs_num * sizeof(JobObject));
	if (series->jobs_array == NULL)
	{
		LOG_ERROR("failed to malloc memory");
		return FALSE;
	}
	for (i = 0; i < jobs_num; i++)
	{
		//The values are not valis in the start on each job place
		series->jobs_array[i].state = EMPTY;
		
		//In the start, the first index of the indexes this job is responsible of
		//is (the index of the job inside the num_jobs array) multiply (job size)
		series->jobs_array[i].starting_index = i * job_size + 1;

		//allocate memory for the values, each array in size of job_size.
		//all arrays of values_arr together are creating the sub_seq_length
		series->jobs_array[i].values_arr = (float *)malloc(job_size * sizeof(float));
		if (series->jobs_array[i].values_arr == NULL)
		{
			LOG_ERROR("failed to malloc memory");
			return FALSE;
		}

		//allocate memory for the values built times
		series->jobs_array[i].built_time_arr = (SYSTEMTIME *)malloc(job_size * sizeof(SYSTEMTIME));
		if (series->jobs_array[i].built_time_arr == NULL)
		{
			LOG_ERROR("failed to malloc memory");
			return FALSE;
		}
	}

	return TRUE;
}

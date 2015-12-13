//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
//depending files: Worker.h, commin.h

//--------Library Includs--------//
#include <stdio.h>
#include <stdlib.h>

//--------Project Includs--------//
#include "Builder.h"
#include "common.h"

//-----Function Declarations ----//


//--------Implementation--------//
BOOL BuildJob(Series *series, int job_id)
{
	int i = 0;
	int a1 = series->a1;
	int d = series->d;
	int q = series->q;
	JobObject *current_job = &(series->jobs_array[job_id]);
	BOOL result = FALSE;
	DWORD thread_id =GetCurrentThreadId();

	current_job->builder_id = thread_id;

	switch (series->type) 
	{
	case ARITHMETIC:
		for (i = 0; i < series->job_size; i++)
		{
			current_job->values_arr[i] = a1 + d * (current_job->starting_index + i - 1);
			// TBD: Set job built time
			//current_job->built_time_arr[i] = 
		}
	default:
		LOG_ERROR("Thread #%d: Found illegal series type (%d)", thread_id, series->type);
		goto cleanup;
	}

	result = TRUE;

cleanup:
	return result;
}

//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
//depending files: Worker.h, commin.h

//--------Library Includs--------//
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <WinBase.h>

//--------Project Includs--------//
#include "Worker.h"
#include "Builder.h"
#include "Cleaner.h"
#include "common.h"

//--------Global Variables---------//
HANDLE work_semaphore;

//-----Function Declarations ----//
BOOL Clean(Series *series, BOOL *has_cleaned_series);

BOOL Build(Series *series);

//--------Implementation--------//
BOOL RunThread (Series *series)
{
	DWORD dwWaitResult;
	BOOL has_cleaned_series = FALSE;
	BOOL retval = FALSE;
	BOOL result = FALSE;

	LOG_INFO("Thread #%d started running", GetCurrentThreadId());
	//while we didn't get to the end of the series in the cleaning job
	while (series->next_job_to_clean < series->jobs_num)
	{
	// Wait for work semaphore until there is work to do
	dwWaitResult = WaitForSingleObject(work_semaphore, INFINITE);
	if (dwWaitResult != WAIT_OBJECT_0) 
	{
		LOG_ERROR("Thread #%d: Failed to wait for work semaphore (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
		goto cleanup;
	}

	if (!Clean(series, &has_cleaned_series))
	{
		LOG_ERROR("Thread #%d: Failed to clean the series", GetCurrentThreadId());
		goto cleanup;
	}

	if (!has_cleaned_series)
	{
		if(!Build(series))
		{
			LOG_ERROR("Thread #%d: Failed to build the series", GetCurrentThreadId());
			goto cleanup;
		}
	}

	// release work semaphore
	retval = ReleaseSemaphore(
		work_semaphore,  // handle to semaphore
		1,				// increase count by one
		NULL);			// not interested in previous count

	if (!retval)
	{
		LOG_ERROR("Thread #%d: Failed to release the work semaphore", GetCurrentThreadId());
		goto cleanup;
	}
	}
	result = TRUE;

cleanup:
	return result;
}


BOOL Clean(Series *series, BOOL *has_cleaned_series)
{
	DWORD dwWaitResult;
	BOOL retval = FALSE;
	BOOL result = FALSE;
	BOOL is_series_ready_to_clean = FALSE;
	int i = 0;
	int curr_job_id = 0;

	LOG_INFO("Thread #%d started cleaning", GetCurrentThreadId());

	// Wait until the building mutex is unlock
	dwWaitResult = WaitForSingleObject(series->mutex_cleaning, INFINITE);
	if (dwWaitResult != WAIT_OBJECT_0)
	{
		LOG_ERROR("Thread #%d: Failed to wait for cleaning mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
		goto cleanup;
	}

	//-----------Cleaning Mutex Critical Section (I)------------//
	is_series_ready_to_clean = (series->cleaning_state == JOBS_TO_CLEAN);
	if (is_series_ready_to_clean)
	{
		series->cleaning_state = CLEANING_IN_PROGRESS;
	}
	//-------End Of Building Mutex Critical Section---------//

	// release the cleaning mutex
	retval = ReleaseMutex(series->mutex_cleaning);
	if (!retval)
	{
		LOG_ERROR(
			"Thread #%d: Failed to release the cleaning mutex of series %d. (ReleaseMutex failed)",
			GetCurrentThreadId(),
			series->type
		);
		goto cleanup;
	}

	// if no job was found, try to build a job in the next series
	if (!is_series_ready_to_clean)
	{
		// TBD: this should allow us to move to the next series
		//continue;
		LOG_INFO("Thread #%d: Nothing to clean in series %d", GetCurrentThreadId(), series->type);
		*has_cleaned_series = FALSE;
		result = TRUE;
		goto cleanup;
	}

	*has_cleaned_series = TRUE;

	curr_job_id = series->next_job_to_clean;
	// verify that the curr_job_id state is effectively DONE
	if (series->jobs_array[curr_job_id].state != DONE)
	{
		LOG_ERROR("Thread #%d: Something Wrong happened. Invariant was invalidated", GetCurrentThreadId());
		goto cleanup;
	}

	while (series->jobs_array[curr_job_id].state == DONE)
	{
		LOG_INFO(
			"Thread #%d started cleaning job id %d with starting index %d", 
			GetCurrentThreadId(), 
			curr_job_id, 
			series->jobs_array[curr_job_id].starting_index
		);
		series->jobs_array[curr_job_id].state = CLEANING;
		CleanJob(series, curr_job_id);
		series->jobs_array[curr_job_id].state = EMPTY;

		// Now check if there was no job to build and set the next_job_to_build accordingly
		// TBD: Do this just one time???
		dwWaitResult = WaitForSingleObject(series->mutex_building, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0)
		{
			LOG_ERROR("Thread #%d: Failed to wait for building mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
			goto cleanup;
		}

		//-----------Building Mutex Critical Section------------//
		if (series->next_job_to_build == -1 && (series->jobs_array[curr_job_id].starting_index<=series->N))//avoiding calcultion of larger than N indexes
		{
			series->next_job_to_build = curr_job_id;
		}
		//-------End Of Building Mutex Critical Section---------//

		// release the building mutex
		retval = ReleaseMutex(series->mutex_building);
		if (!retval)
		{
			LOG_ERROR(
				"Thread #%d: Failed to release the building mutex of series %d. (ReleaseMutex failed)",
				GetCurrentThreadId(),
				series->type
			);
			goto cleanup;
		}

		// increment the curr_job_id to try cleaning the next job (if DONE)
		curr_job_id++;
		if (curr_job_id=series->jobs_num) //TBD:cyclic??
		{
			break; //if we were on the last job to clean->break
		}
		curr_job_id=curr_job_id%(series->jobs_num);
	}
		
	// Now change the cleaning state to NOTHING_TO_CLEAN
	dwWaitResult = WaitForSingleObject(series->mutex_cleaning, INFINITE);
	if (dwWaitResult != WAIT_OBJECT_0)
	{
		LOG_ERROR("Thread #%d: Failed to wait for cleaning mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
		goto cleanup;
	}

	//-----------Cleaning Mutex Critical Section------------//
	series->next_job_to_clean = curr_job_id;
	series->cleaning_state = NOTHING_TO_CLEAN;
	//-------End Of Cleaning Mutex Critical Section---------//

	// release the cleaning mutex
	retval = ReleaseMutex(series->mutex_cleaning);
	if (!retval)
	{
		LOG_ERROR(
			"Thread #%d: Failed to release the cleaning mutex of series %d. (ReleaseMutex failed)",
			GetCurrentThreadId(),
			series->type
		);
		goto cleanup;
	}
	
	result = TRUE;

cleanup:
	return result;

}


BOOL Build(Series *series)
{
	DWORD dwWaitResult;
	BOOL retval = FALSE;
	BOOL result = FALSE;
	BOOL has_found_job_in_series = FALSE;
	int curr_job_id = -1;
	int i = 0;
	int j = 0;
	int k = 0;

	LOG_INFO("Thread #%d started building", GetCurrentThreadId());

	// Wait until the building mutex is unlock
	dwWaitResult = WaitForSingleObject(series->mutex_building, INFINITE);
	if (dwWaitResult != WAIT_OBJECT_0)
	{
		LOG_ERROR("Thread #%d: Failed to wait for building mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
		goto cleanup;
	}

	//-----------Building Mutex Critical Section------------//
	has_found_job_in_series = (series->next_job_to_build != -1);

	if (series->next_job_to_build != -1)
	{	
		// Found a job to build
		curr_job_id = series->next_job_to_build;
		series->jobs_array[curr_job_id].state = BUILDING;
		LOG_INFO(
			"Thread #%d started building job id %d with starting index %d", 
			GetCurrentThreadId(), 
			curr_job_id, 
			series->jobs_array[curr_job_id].starting_index
		);

		// search for a new job to build
		series->next_job_to_build = -1;
		j = curr_job_id + 1;
		for (k = 0; k < series->jobs_num; k++)
		{//(series->jobs_array[j].starting_index<=series->N) we don't want to build indexes larger than N
			if (series->jobs_array[j].state == EMPTY && (series->jobs_array[j].starting_index<=series->N))
			{
				series->next_job_to_build = j;
				LOG_INFO("Thread #%d: set next_job_to_build to %d", GetCurrentThreadId(), j);
				break;
			}
			j = (j+1) % series->jobs_num;
		}
	}
	//-------End Of Building Mutex Critical Section---------//

	// release the building mutex
	retval = ReleaseMutex(series->mutex_building);
	if (!retval)
	{
		LOG_ERROR(
			"Thread #%d: Failed to release the building mutex of series %d. (ReleaseMutex failed)",
			GetCurrentThreadId(),
			series->type
		);
		goto cleanup;
	}

	// if no job was found, try to build a job in the next series
	if (!has_found_job_in_series)
	{
		// TBD: this should allow us to move to the next series
		//continue;
		LOG_INFO("Thread #%d: Nothing to build in series %d", GetCurrentThreadId(), series->type);
		result = TRUE;
		goto cleanup;
	}

	BuildJob(series, curr_job_id);
	series->jobs_array[curr_job_id].state = DONE;

	// Now check if the cleaning state should be changed
	dwWaitResult = WaitForSingleObject(series->mutex_cleaning, INFINITE);
	if (dwWaitResult != WAIT_OBJECT_0)
	{
		LOG_ERROR("Thread #%d: Failed to wait for cleaning mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
		goto cleanup;
	}

	//-----------Cleaning Mutex Critical Section------------//
	if (series->cleaning_state == NOTHING_TO_CLEAN)
	{
		if (series->next_job_to_clean == curr_job_id)
		{
			series->cleaning_state = JOBS_TO_CLEAN;
		}
	}
	//-------End Of Cleaning Mutex Critical Section---------//

	// release the cleaning mutex
	retval = ReleaseMutex(series->mutex_cleaning);
	if (!retval)
	{
		LOG_ERROR(
			"Thread #%d: Failed to release the cleaning mutex of series %d. (ReleaseMutex failed)",
			GetCurrentThreadId(),
			series->type
		);
		goto cleanup;
	}
	
	result = TRUE;

cleanup:
	return result;
}

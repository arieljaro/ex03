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
BOOL Clean(Series *series_arr, BOOL *has_cleaned_series, BOOL *are_all_cleaning_states_completed_ptr);

BOOL Build(Series *series_arr);

//--------Implementation--------//
BOOL RunThread (Series *series_arr)
{
	DWORD dwWaitResult;
	BOOL has_cleaned_series = FALSE;
	BOOL are_all_cleaning_states_completed = FALSE;
	BOOL retval = FALSE;
	BOOL result = FALSE;

	LOG_DEBUG("Thread #%d started running", GetCurrentThreadId());
	
	//while we didn't get to the end of the series in the cleaning job
	while (!are_all_cleaning_states_completed)
	{
		LOG_DEBUG("Thread #%d is about to enter the semaphore", GetCurrentThreadId());

		// Wait for work semaphore until there is work to do
		dwWaitResult = WaitForSingleObject(work_semaphore, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0) 
		{
			LOG_ERROR("Thread #%d: Failed to wait for work semaphore (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
			goto cleanup;
		}
		LOG_DEBUG("Thread #%d passed the semaphore", GetCurrentThreadId());

		if (!Clean(series_arr, &has_cleaned_series, &are_all_cleaning_states_completed))
		{
			LOG_ERROR("Thread #%d: Failed to clean the series", GetCurrentThreadId());
			goto cleanup;
		}

		if (!has_cleaned_series)
		{
			if(!Build(series_arr))
			{
				LOG_ERROR("Thread #%d: Failed to build the series", GetCurrentThreadId());
				goto cleanup;
			}
		}

		LOG_DEBUG("Thread #%d completed an iteration", GetCurrentThreadId());
	}

	result = TRUE;

cleanup:
	LOG_DEBUG("Thread #%d finnished running with exit code %d", GetCurrentThreadId(), result);
	return result;
}


BOOL Clean(Series *series_arr, BOOL *has_cleaned_series, BOOL *are_all_cleaning_states_completed_ptr)
{
	CleaningState cleaning_state;
	DWORD dwWaitResult;
	BOOL retval = FALSE;
	BOOL result = FALSE;
	BOOL is_series_ready_to_clean = FALSE;
	BOOL are_all_cleaning_states_completed = TRUE;
	BOOL should_release_work_semaphore = FALSE;
	BOOL should_release_work_semaphore_at_cleanup = FALSE;
	BOOL release_failed = FALSE;
	int i = 0;
	int j = 0;
	int curr_job_id = 0;
	SeriesType series_type;
	Series *series = NULL;

	LOG_DEBUG("Thread #%d started cleaning", GetCurrentThreadId());

	for (j = 0; j < SERIES_TYPES_COUNT; j++)
	{
		series_type = (SeriesType)j;
		series = &(series_arr[series_type]);

		_try {
			// Wait until the building mutex is unlock
			dwWaitResult = WaitForSingleObject(series->mutex_cleaning, INFINITE);
			if (dwWaitResult != WAIT_OBJECT_0)
			{
				LOG_ERROR("Thread #%d: Failed to wait for cleaning mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
				goto cleanup;
			}
	
			//-----------Cleaning Mutex Critical Section (I)------------//
			cleaning_state = series->cleaning_state;
			is_series_ready_to_clean = (cleaning_state == JOBS_TO_CLEAN);
			if (is_series_ready_to_clean)
			{
				series->cleaning_state = CLEANING_IN_PROGRESS;
			}
			//-------End Of Building Mutex Critical Section---------//
		}
		_finally {
			// release the cleaning mutex
			retval = ReleaseMutex(series->mutex_cleaning);
			if (!retval)
			{
				LOG_ERROR(
					"Thread #%d: Failed to release the cleaning mutex of series %d. (ReleaseMutex failed)",
					GetCurrentThreadId(),
					series->type
				);
				release_failed = TRUE;
			}
		}
		if (release_failed)
		{
			goto cleanup;
		}

		are_all_cleaning_states_completed = are_all_cleaning_states_completed && (cleaning_state == CLEANING_COMPLETED);

		if (is_series_ready_to_clean)
		{
			break;
		}
	}

	// if no job was found, try to build a job in the next series
	if (!is_series_ready_to_clean)
	{
		LOG_DEBUG("Thread #%d: Nothing to clean in series %d", GetCurrentThreadId(), series->type);
		*has_cleaned_series = FALSE;
		if (are_all_cleaning_states_completed)
		{
			*are_all_cleaning_states_completed_ptr = TRUE;
			should_release_work_semaphore_at_cleanup = TRUE;
		}
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
		LOG_DEBUG(
			"Thread #%d started cleaning job id %d in series #%d with starting index %d", 
			GetCurrentThreadId(), 
			curr_job_id,
			series_type,
			series->jobs_array[curr_job_id].starting_index
		);
		series->jobs_array[curr_job_id].state = CLEANING;
		CleanJob(series, curr_job_id);

		// advance the job starting index by sub_seq_length (== jobs_num * job_size)
		series->jobs_array[curr_job_id].starting_index += series->jobs_num * series->job_size;
		if (series->jobs_array[curr_job_id].starting_index > series->N)
		{
			LOG_DEBUG("series->jobs_array[curr_job_id].starting_index (%d) >= series->N", series->jobs_array[curr_job_id].starting_index);
			series->jobs_array[curr_job_id].state = COMPLETED;
		} else {
			series->jobs_array[curr_job_id].state = EMPTY;
			should_release_work_semaphore = TRUE;
		}

		_try {
			// Now check if there was no job to build and set the next_job_to_build accordingly
			dwWaitResult = WaitForSingleObject(series->mutex_building, INFINITE);
			if (dwWaitResult != WAIT_OBJECT_0)
			{
				LOG_ERROR("Thread #%d: Failed to wait for building mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
				goto cleanup;
			}

			//-----------Building Mutex Critical Section------------//
			if ((series->next_job_to_build == -1) && (series->jobs_array[curr_job_id].state == EMPTY))
			{
				series->next_job_to_build = curr_job_id;
			}
			//-------End Of Building Mutex Critical Section---------//
		}
		_finally {
			// release the building mutex
			retval = ReleaseMutex(series->mutex_building);
			if (!retval)
			{
				LOG_ERROR(
					"Thread #%d: Failed to release the building mutex of series %d. (ReleaseMutex failed)",
					GetCurrentThreadId(),
					series->type
				);
				release_failed = TRUE;
			}
		}
		if (release_failed)
		{
			goto cleanup;
		}

		if (should_release_work_semaphore)
		{
			// release work semaphore to start a builder
			retval = ReleaseSemaphore(
				work_semaphore,  // handle to semaphore
				1,				// increase count by one
				NULL);			// not interested in previous count

			if (!retval)
			{
				LOG_ERROR("Thread #%d: Failed to release the work semaphore", GetCurrentThreadId());
				goto cleanup;
			}

			should_release_work_semaphore = FALSE;
		}

		// increment the curr_job_id to try cleaning the next job (if DONE)
		curr_job_id = (curr_job_id + 1) % (series->jobs_num);
	}
	
	_try {
		// Now change the cleaning state to NOTHING_TO_CLEAN/CLEANING_COMPLETED
		dwWaitResult = WaitForSingleObject(series->mutex_cleaning, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0)
		{
			LOG_ERROR("Thread #%d: Failed to wait for cleaning mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
			goto cleanup;
		}

		//-----------Cleaning Mutex Critical Section------------//
		series->next_job_to_clean = curr_job_id;
		if (series->jobs_array[curr_job_id].state == COMPLETED)
		{
			series->cleaning_state = CLEANING_COMPLETED;
			should_release_work_semaphore_at_cleanup = TRUE;
			// here it might be the case that we shoul update the are_all_cleaning_states_completed_ptr flag,
			// but we prefer not to update it since it demands us to check the cleaning state of the other series
		} else {
			series->cleaning_state = NOTHING_TO_CLEAN;
		}
		//-------End Of Cleaning Mutex Critical Section---------//
	}
	_finally {
		// release the cleaning mutex
		retval = ReleaseMutex(series->mutex_cleaning);
		if (!retval)
		{
			LOG_ERROR(
				"Thread #%d: Failed to release the cleaning mutex of series %d. (ReleaseMutex failed)",
				GetCurrentThreadId(),
				series->type
			);
			release_failed = TRUE;
		}
	}
	if (release_failed)
	{
		goto cleanup;
	}

	result = TRUE;

cleanup:
	if (should_release_work_semaphore_at_cleanup)
	{
		retval = ReleaseSemaphore(
			work_semaphore,				// handle to semaphore
			1,							// release the semaphore for all the threads
			NULL);						// not interested in previous count
		
		if (!retval)
		{
			LOG_ERROR("Thread #%d: Failed to release the work semaphore", GetCurrentThreadId());
			goto cleanup;
		}
	}
	return result;

}


BOOL Build(Series *series_arr)
{
	Series *series;
	SeriesType series_type;
	DWORD dwWaitResult;
	BOOL retval = FALSE;
	BOOL result = FALSE;
	BOOL has_found_job_in_series = FALSE;
	BOOL should_release_work_semaphore = FALSE;
	BOOL release_failed = FALSE;
	int curr_job_id = -1;
	int i = 0;
	int j = 0;
	int k = 0;

	LOG_DEBUG("Thread #%d started building", GetCurrentThreadId());

	// Search for a series to build on
	for (i = 0; i < SERIES_TYPES_COUNT; i++)
	{
		series_type = (SeriesType)i;
		series = &series_arr[series_type];

		_try {
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
				LOG_DEBUG(
					"Thread #%d started building job id %d in series #%d with starting index %d", 
					GetCurrentThreadId(), 
					curr_job_id,
					series_type,
					series->jobs_array[curr_job_id].starting_index
				);

				// search for a new job to build in the series
				series->next_job_to_build = -1;
				j = (curr_job_id + 1) % (series->jobs_num);
				for (k = 0; k < series->jobs_num; k++)
				{
					if (series->jobs_array[j].state == EMPTY)
					{
						series->next_job_to_build = j;
						LOG_DEBUG("Thread #%d: set next_job_to_build to %d", GetCurrentThreadId(), j);
						break;
					}
					j = (j+1) % series->jobs_num;
				}
			}
			//-------End Of Building Mutex Critical Section---------//
		}
		_finally {
			// release the building mutex
			retval = ReleaseMutex(series->mutex_building);
			if (!retval)
			{
				LOG_ERROR(
					"Thread #%d: Failed to release the building mutex of series %d. (ReleaseMutex failed)",
					GetCurrentThreadId(),
					series->type
				);
				release_failed = TRUE;
			}
		}
		if (release_failed)
		{
			goto cleanup;
		}

		if (has_found_job_in_series)
		{
			break;
		}

	}

	// if no job was found, try to build a job in the next series
	if (!has_found_job_in_series)
	{
		LOG_DEBUG("Thread #%d: Nothing to build in any series", GetCurrentThreadId(), series->type);
		result = TRUE;
		goto cleanup;
	}

	BuildJob(series, curr_job_id);
	series->jobs_array[curr_job_id].state = DONE;

	_try {
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
				// in this case release the work semaphore to allow a sleeping thread to be awakened to start cleaning
				should_release_work_semaphore = TRUE;
			}
		}
		//-------End Of Cleaning Mutex Critical Section---------//
	}
	_finally {
		// release the cleaning mutex
		retval = ReleaseMutex(series->mutex_cleaning);
		if (!retval)
		{
			LOG_ERROR(
				"Thread #%d: Failed to release the cleaning mutex of series %d. (ReleaseMutex failed)",
				GetCurrentThreadId(),
				series->type
			);
			release_failed = TRUE;
		}
	}
	if (release_failed)
	{
		goto cleanup;
	}

	if (should_release_work_semaphore)
	{
		// release work semaphore to start a cleaner
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

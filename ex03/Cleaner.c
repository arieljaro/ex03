//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
//depending files: Worker.h, commin.h

//--------Library Includs--------//
#include <stdio.h>
#include <stdlib.h>

//--------Project Includs--------//
#include "Cleaner.h"
#include "common.h"

//--------Implementation--------//
BOOL CleanJob(Series *series, int job_id)
{
	BOOL result = FALSE;
	int index_within_series = 0;
    int index_within_sector = 0;
	JobObject *job = NULL;
 
    //checking the validity of the given inputs:
	if ((series == NULL) || (job_id >= series->jobs_num))
    {
        LOG_ERROR("Wrong paramters");
        goto cleanup;
    }

	job = &(series->jobs_array[job_id]);

    //printing the data of all elements in the sector to the output file
	for (index_within_sector = 0; index_within_sector < series->job_size; index_within_sector++)
    {
        //calculating the element index in series:
		index_within_series = job->starting_index + index_within_sector;

		fprintf(
			series->output_file, 
			"Index #%d = %f, calculated by thread %ld @ %02d:%02d:%02d.%3d\n", 
			index_within_series,
			job->values_arr[index_within_sector],
			job->builder_id,
			job->built_time_arr[index_within_sector].wHour,
			job->built_time_arr[index_within_sector].wMinute,
			job->built_time_arr[index_within_sector].wSecond,
			job->built_time_arr[index_within_sector].wMilliseconds
		);

		// clean value
		job->values_arr[index_within_sector] = 0;
    }
 
	result = TRUE;
cleanup:
    return result;

}

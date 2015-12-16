//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
//depending files: common.h
#ifndef WORKER_H
#define WORKER_H

//--------Library Includes--------//
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <WinBase.h>

//--------Project Includes--------//
#include "common.h"

//--------Definitions--------//
#define ARITHMETIC_OUTPUT_FILENAME "arithmetic.txt"
#define GEOMETRIC_OUTPUT_FILENAME "geometric.txt"
#define DIFFERENTIAL_OUTPUT_FILENAME "differential.txt"

typedef enum{
	EMPTY=0,	//the data on the JobObject is not valid and no worker is working on this Job(see JobObject defintion) 
	BUILDING,	//there is a Worker working on building the value associated with this JobObject
	DONE,		//the data on the JobObject is valid and no worker is working on this Job
	CLEANING,	//there is a Worker working on cleaning the value associated with this JobObject
	COMPLETED	//the job is done (starting_index > N)
} JobState;

typedef enum{
	NOTHING_TO_CLEAN=0,		//There is nothing to clean in this series
	CLEANING_IN_PROGRESS,	//There is one worker cleaning this series
	JOBS_TO_CLEAN,			//There is one or more Jobs for cleaning in this series and no worker is already cleaning.
	CLEANING_COMPLETED		// Finished cleaning all the series
} CleaningState;

typedef struct JobObject_s{
	JobState state;
	DWORD builder_id;	 //Thread Is of the builder which built this values in the array
	int starting_index;
	float *values_arr;	 //The values of the series
						 //Each job contaning diffrent range in size of job_size,
						 //where the relevant terms will be stored
	SYSTEMTIME *built_time_arr;
} JobObject;

typedef JobObject *JobArray;

typedef enum {
	ARITHMETIC_SERIES = 0,
	GEOMETRIC_SERIES,
	DIFFERENTIAL_SERIES,
	SERIES_TYPES_COUNT
} SeriesType;

typedef struct Series_s{
	SeriesType type;				// The series type
	JobArray jobs_array;			// containg the Jobs array for the specific series. 
	CleaningState cleaning_state; 
	volatile HANDLE mutex_cleaning;			// A handle to the mutex for accessing cleaning related indicators
	int next_job_to_clean;			// The next job number which a cleaner has to clean.
	volatile HANDLE mutex_building;			// A handle to the mutex for accessing buildng related indicators
	int next_job_to_build;			// Next job number which a builder has to built 
	FILE *output_file;
	int job_size;
	int jobs_num;
	float a1;
	float d;
	float q;
	int N;
	unsigned int semaphore_size;
} Series;

//--------Function Declarations--------//
BOOL RunThread (Series *series);

// TBD: remove this - needed for phase 1 only
//BOOL Clean (Series *series);

#endif //WORKER_H

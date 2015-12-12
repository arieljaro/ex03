//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
//depending files: 
#ifndef WORKER_H
#define WORKER_H
//--------Library Includs--------//

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <WinBase.h>
//--------Project Includs--------//
//--------Definations--------//
typedef enum{
	EMPTY=0,	//the data on the JobObject is not valid and no worker is working on this Job(see JobObject defintion) 
	BUILDING,	//there is a Worker working on building the value associated with this JobObject
	DONE,		//the data on the JobObject is valid and no worker is working on this Job
	CLEANING	//there is a Worker working on cleaning the value associated with this JobObject
} JobState;

typedef enum{
	NOTHING_TO_CLEAN=0, //There is nothing to clean in this series
	CLEANING,			//There is one worker cleaning this series
	JOBS_TO_CLEAN,		//There is one or more Jobs for cleaning in this series and no worker is already cleaning.
} CleaningState;

typedef struct JobObject_s{
	JobState state;
	DWORD builder_id;	 //Thread Is of the builder which built this values in the array
	unsigned int starting_index;
	int *values_arr;	 //The values of the series
	SYSTEMTIME *built_time_arr;
} JobObject;

typedef JobObject *JobArray;

typedef struct Series_s{
	JobArray jobs_array;			//containg the Jobs array for the specific series. 
	CleaningState cleaning_state; 
	HANDLE mutex_cleaning;			//A handle to the mutex for accessing cleaning related indicators
	unsigned int next_job_to_clean; //The next job number which a cleaner has to clean.
	HANDLE mutex_building;			//A handle to the mutex for accessing buildng related indicators
	unsigned int next_job_to_build; //Next job number which a builder has to built 
} Series;

//--------Function Declarations--------//
BOOL RunThread (Series series); 
#endif //WORKER_H
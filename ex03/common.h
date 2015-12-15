#ifndef COMMON_H
#define COMMON_H

//--------Library Includes--------//
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <stdlib.h>
#include <WinBase.h>
#include <tchar.h>

//--------Definitions--------//
#define LOG_ERROR(msg, ...) fprintf(stderr, "[ERROR] (%s:%d) - " msg "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(msg, ...) fprintf(stderr, "[WARNING] (%s:%d) - " msg "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) fprintf(stderr, "[INFO] (%s:%d) - " msg "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum {
	SUCCESS = 0,
	GENERAL_FAILURE,
	WRONG_PARAMETERS,
	MALLOC_FAILED,
	CREATE_SEMAPHORE_FAILED,
	INTIALIZE_SERIES_FAILED,
	THREAD_CREATION_FAILED,
	THREAD_RUN_FAILED,
	WAIT_FOR_MULTIPLE_OBJECT_FAILED
} ErrorCode;

#endif //COMMON_H

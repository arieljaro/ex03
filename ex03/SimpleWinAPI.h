//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
#ifndef __SIMPLE_WIN_API_H
#define __SIMPLE_WIN_API_H

//--------Library Includes--------//
#include <Windows.h>

//--------Function Declarations--------//
/**
 * This function creates a thread by calling Win32 Api's CreateThread()
 * function, and setting some of the parameters to default value
 * (security arguments, stack size and creation flags)
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
HANDLE CreateThreadSimple(
	LPTHREAD_START_ROUTINE StartAddress,
	LPVOID ParameterPtr,
	LPDWORD ThreadIdPtr);

/* Creates a process with default arguments (no module name, hanlde not inheritable, thread handle not inheritable,
 * handle inheritance is set to FALSE, normal priority class, parent's environment block and starting directory
 * and neutral startup info parameter). 
 * All the other parameters are passed as-is to CreateProcess 
 */
BOOL CreateProcessSimple(
	LPTSTR CommandLine, 
	PROCESS_INFORMATION *ProcessInfoPtr);

#endif

//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
//--------Project Includes--------//
#include "SimpleWinAPI.h"

HANDLE CreateThreadSimple(
	LPTHREAD_START_ROUTINE StartAddress,
	LPVOID ParameterPtr,
	LPDWORD ThreadIdPtr)
{
	return CreateThread(
		NULL,            /*  default security attributes */
		0,                /*  use default stack size */
		StartAddress,    /*  thread function */
		ParameterPtr,    /*  argument to thread function */
		0,                /*  use default creation flags */
		ThreadIdPtr);    /*  returns the thread identifier */	
}


BOOL CreateProcessSimple(
	LPTSTR CommandLine, 
	PROCESS_INFORMATION *ProcessInfoPtr) 
{
	STARTUPINFO    startinfo = { sizeof(STARTUPINFO), NULL, 0 }; /* <ISP> here we */
																 /* initialize a "Neutral" STARTUPINFO variable. Supplying this to */
																 /* CreateProcess() means we have no special interest in this parameter. */
																 /* This is equivalent to what we are doing by supplying NULL to most other */
																 /* parameters of CreateProcess(). */

	return CreateProcess(NULL, /*  No module name (use Command line). */
		CommandLine,            /*  Command line. */
		NULL,                    /*  Process handle not inheritable. */
		NULL,                    /*  Thread handle not inheritable. */
		FALSE,                    /*  Set handle inheritance to FALSE. */
		NORMAL_PRIORITY_CLASS,    /*  creation/priority flags. */
		NULL,                    /*  Use parent's environment block. */
		NULL,                    /*  Use parent's starting directory. */
		&startinfo,                /*  Pointer to STARTUPINFO structure. */
		ProcessInfoPtr            /*  Pointer to PROCESS_INFORMATION structure. */
		);
}

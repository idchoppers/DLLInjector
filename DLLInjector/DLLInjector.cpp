/*
	Name	    : DLLInjector.cpp
	Author	    : idchoppers
	Date	    : 1/27/2024
	Description : Injects a DLL into a running process.
*/

/* Includes/Defines ************************************************************************/

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

/* Global Vars *****************************************************************************/

constexpr auto BUFFSIZE = 32767;

/* Prototypes ******************************************************************************/

// Gets the ID of the target process
DWORD 
GetProcId (const wchar_t* procName);

// Gets the DLL to inject
LPWSTR
GetDLLFilePath ();

/* Main ************************************************************************************/

int
main (int argc, const char* argv[])
{
	DWORD procID = {};
	if (argc <= 1)
	{
		std::cout << "[USAGE] Please specify the image name of the running program. Ex: Taskmgr.exe" << std::endl;
		return EXIT_FAILURE;
	}
	size_t imageLength = strlen (argv[1]);
	wchar_t procName[MAX_PATH];
	mbstowcs_s (&imageLength, procName, argv[1], imageLength);

	LPWSTR DLLPath = GetDLLFilePath ();
	int pathLength = WideCharToMultiByte(CP_ACP, 0, DLLPath, -1, 0, 0, NULL, NULL);
	char* pathName = new char[pathLength];
	WideCharToMultiByte(CP_ACP, 0, DLLPath, -1, pathName, pathLength, NULL, NULL);

	std::cout << "[STATUS] Found DLL at " << pathName << std::endl;

	while (!procID)
	{
		std::cout << "[STATUS] Trying to find process ID for " << argv[1] << std::endl;
		procID = GetProcId (procName);
		Sleep (30);
	}
	std::wcout << "[STATUS] Found ID " << procID << " for process " << procName << std::endl;
	
	HANDLE procHandle = OpenProcess (PROCESS_ALL_ACCESS, 0, procID);

	if (procHandle != INVALID_HANDLE_VALUE)
	{
		LPVOID procLocation = VirtualAllocEx (procHandle, 0, BUFFSIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (procLocation != 0 && WriteProcessMemory (procHandle, procLocation, pathName, strlen (pathName) + 1, 0) != TRUE)
		{
			std::cout << "[ERROR] Failed to write DLL to process memory! Error( " << GetLastError () << " )" << std::endl;
			return EXIT_FAILURE;
		}
		std::cout << "[STATUS] Wrote DLL to process memory at " << std::hex << procLocation << std::endl;

		HANDLE threadHandle = CreateRemoteThread (procHandle, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, procLocation, 0, 0);
		std::cout << "[STATUS] Library loaded, cleaning up..." << std::endl;

		if (threadHandle)
		{
			CloseHandle (threadHandle);
		}
	}

	delete[] pathName;
	
	if (procHandle)
	{
		CloseHandle (procHandle);
	}

	std::cout << "[STATUS] Done injecting DLL!" << std::endl;
	return EXIT_SUCCESS;
}

/* Defenitions *****************************************************************************/

/*
	Finds and returns the process ID given its image name.
	Input:
	procName The target process image name
	Output:
	Returns the running process ID
*/
DWORD
GetProcId (const wchar_t* procName)
{
	DWORD procId = {};
	HANDLE snapshotHandle = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

	if (snapshotHandle != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof (procEntry);

		if (Process32First (snapshotHandle, &procEntry))
		{
			do
			{
				if (!wcscmp (procEntry.szExeFile, procName))
				{
					procId = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next (snapshotHandle, &procEntry));
		}
	}
	CloseHandle (snapshotHandle);

	return procId;
}

/*
	Gets the DLL file handle from the user using an open file dialog.
	Output:
	File Handle
*/
LPWSTR
GetDLLFilePath ()
{
	OPENFILENAME open;
	wchar_t* pathSize;
	LPWSTR filePath = {};

	pathSize = (wchar_t*)malloc(BUFFSIZE);

	ZeroMemory (&open, sizeof (open));
	open.lStructSize = sizeof (OPENFILENAME);
	open.lpstrFile = pathSize;
	open.lpstrFile[0] = '\0';
	open.nMaxFile = sizeof (pathSize);
	open.lpstrFilter = L"DLL Files\0*.DLL\0\0";
	open.nFileOffset = 1;
	open.lpstrFileTitle = NULL;
	open.lpstrInitialDir = NULL;
	open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	
	free(pathSize);
	
	if (GetOpenFileName(&open))
	{
		filePath = open.lpstrFile;
	}

	return filePath;
}
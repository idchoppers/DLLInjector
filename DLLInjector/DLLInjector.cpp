/*
	Name	    : DLLInjector.cpp
	Author	    : idchoppers
	Date	    : 1/28/2024
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
GetProcID (const wchar_t* procName);

// Gets the DLL to inject
const char*
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

	const char* pathName = GetDLLFilePath ();
	if (pathName == NULL)
	{
		std::cout << "[ERROR] DLL File not found!" << std::endl;
		delete[] pathName;
		pathName = nullptr;
		return EXIT_FAILURE;
	}
	std::cout << "[STATUS] Found DLL at " << pathName << std::endl;

	procID = GetProcID (procName);
	if (procID == 0)
	{
		std::cout << "[ERROR] Process not found! Make sure name of process is correct and that it is running!" << std::endl;
		delete[] pathName;
		pathName = nullptr;
		return EXIT_FAILURE;
	}
	std::wcout << "[STATUS] Found ID " << procID << " for process " << procName << std::endl;
	
	HANDLE procHandle = OpenProcess (PROCESS_ALL_ACCESS, 0, procID);

	if (procHandle != INVALID_HANDLE_VALUE)
	{
		LPVOID procLocation = VirtualAllocEx (procHandle, 0, BUFFSIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (procLocation != 0 && pathName != 0
			&& WriteProcessMemory (procHandle, procLocation, pathName, strlen (pathName) + 1, 0) != TRUE)
		{
			std::cout << "[ERROR] Failed to write DLL to process memory! Error( " << GetLastError () << " )" << std::endl;
			delete[] pathName;
			pathName = nullptr;
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
	pathName = nullptr;

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
GetProcID (const wchar_t* procName)
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
const char*
GetDLLFilePath ()
{
	OPENFILENAME open;
	LPWSTR filePath = {};
	char* pathName = {};
	int pathLength = {};

	wchar_t* pathSize = new wchar_t[BUFFSIZE]();

	ZeroMemory (&open, sizeof (open));
	open.lStructSize = sizeof (OPENFILENAME);
	open.lpstrFile = pathSize;
	open.lpstrFile[0] = '\0';
	open.nMaxFile = BUFFSIZE;
	open.lpstrFilter = L"DLL Files\0*.DLL\0\0";
	open.nFileOffset = 1;
	open.lpstrFileTitle = NULL;
	open.lpstrInitialDir = NULL;
	open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName (&open))
	{
		filePath = open.lpstrFile;

		pathLength = WideCharToMultiByte (CP_ACP, 0, filePath, -1, 0, 0, NULL, NULL);
		pathName = new char[pathLength];
		WideCharToMultiByte (CP_ACP, 0, filePath, -1, pathName, pathLength, NULL, NULL);
	}

	delete[] pathSize;
	pathSize = nullptr;

	return pathName;
}
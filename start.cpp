#include <Windows.h>
#include <shellapi.h>
#include <stdexcept>
#include <iostream>

int _main(HINSTANCE hinstance)
{
	try
	{
		//Get directories
		std::wstring directory(64, '\0');
		while (true)
		{
			DWORD read = GetModuleFileName(hinstance, &directory[0], directory.size());
			if (read == 0) throw std::runtime_error("GetModuleFileName failed");
			else if (read == directory.size()) directory.resize(2 * directory.size(), '\0');
			else break;
		}
		std::string::size_type slash = directory.rfind('\\');
		directory.resize(slash + 1);
		std::wstring packaged_slicer = directory + L"core.exe";
		bool packaged = GetFileAttributes(packaged_slicer.c_str()) != INVALID_FILE_ATTRIBUTES;
		std::wstring pdirectory, ppdirectory;
		pdirectory = directory;
		if ((slash = pdirectory.rfind('\\', pdirectory.size() - 2)) != std::wstring::npos)
		{
			pdirectory.resize(slash + 1);
			ppdirectory = pdirectory;
			if ((slash = ppdirectory.rfind('\\', ppdirectory.size() - 2)) != std::wstring::npos)
			{
				ppdirectory.resize(slash + 1);
			}
		}
		
		//Parse arguments
		int argc;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
		if (argv == nullptr || argc == 0) throw std::runtime_error("CommandLineToArgvW failed");

		//Argument 0
		std::wstring program = argv[0];
		if (program.substr(program.size() - 4) != L".exe") throw std::runtime_error("Argument 0 is not an executable");

		//Argument 1
		std::wstring target;
		bool target_is_first_argument = false;
		if (argc >= 2)
		{
			std::wstring first_argument = argv[1];
			if (first_argument.substr(first_argument.size() - 4) == L".exe")
			{
				target = first_argument;
				target_is_first_argument = true;
			}
		}
		if (target.empty())
		{
			target = packaged ? packaged_slicer : (ppdirectory + L"application\\Slicer\\bin\\x64\\Debug\\Slicer.exe");
		}

		//Other arguments
		std::wstring command;
		for (int i = target_is_first_argument ? 2 : 1; i < argc; i++)
		{
			command += argv[i];
			if (i != argc - 1) command += L" ";
		}
		LocalFree(argv);

		//Environment
		TCHAR *environemnt = GetEnvironmentStrings();
		if (environemnt == nullptr) throw std::runtime_error("GetEnvironmentStrings failed");
		std::wstring new_environment;
		for (TCHAR* environemnt_iter = environemnt; environemnt_iter[0] != '\0'; environemnt_iter += (wcslen(environemnt_iter) + 1))
		{
			std::wstring entry = environemnt_iter;
			std::wstring entry5 = entry.substr(0, 5);
			if (_wcsicmp(entry5.c_str(), L"PATH=") == 0)
			{
				std::wstring old_entry = entry.substr(5);
				std::wstring first_party = packaged ? (directory) : (pdirectory + L"Release");
				std::wstring second_party = packaged ? (directory + L"second-party") : (ppdirectory + L"application\\Slicer\\bin\\x64\\Debug");
				std::wstring third_party_directory = packaged ? (directory + L"third-party") : (ppdirectory + L"third-party");
				std::wstring third_party =
					third_party_directory + L"\\boost;" +
					third_party_directory + L"\\ffmpeg;" +
					third_party_directory + L"\\freeimage;" +
					third_party_directory + L"\\freetype;" +
					third_party_directory + L"\\glew;" +
					third_party_directory + L"\\jemalloc;" +
					third_party_directory + L"\\opencascade;" +
					third_party_directory + L"\\openvr;" +
					third_party_directory + L"\\tbb";
				std::wstring value = first_party + L";" + second_party + L";" + third_party + L";" + old_entry;
				SetEnvironmentVariable(L"PATH", value.c_str());
				entry = L"PATH=" + value;
			}
			new_environment += entry;
			new_environment.push_back('\0');
		}
		new_environment.push_back('\0');
		FreeEnvironmentStrings(environemnt);
		
		//Start process
		std::wstring target_directory = target;
		slash = target.rfind('\\');
		target_directory.resize(slash);
		STARTUPINFO si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		if (!CreateProcess(&target[0], &command[0], nullptr, nullptr, true, CREATE_UNICODE_ENVIRONMENT, &new_environment[0], &target_directory[0], &si, &pi))
			throw std::runtime_error("CreateProcess failed");
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD code;
		GetExitCodeProcess(pi.hProcess, &code);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return code;
	}
	catch (const std::exception &e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
		return 1;
	}
    return 0;
}

#ifdef WIN32_EXECUTABLE
    int WINAPI WinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE, _In_ PSTR, _In_ int)
	{
		return _main(hinstance);
	}
#else
	int main()
	{
		return _main(GetModuleHandle(NULL));
	}
#endif
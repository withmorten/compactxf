#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#define strequal(_Str1, _Str2) !strcmp(_Str1, _Str2)

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

typedef uint8 byte;

typedef uint8 bool8;
typedef int32 bool32;

typedef uintptr_t uintptr;
typedef ptrdiff_t ptrdiff;

using std::string;
using std::vector;

vector<string> ignore_exts;
vector<string> files;

static void fill_ignore_exts(char *ignore_file_name)
{
	FILE *file = fopen(ignore_file_name, "r");

	if (file)
	{
		char line[2048];

		while (fgets(line, sizeof(line), file))
		{
			if (*line == ';') continue;

			for (char *p = line; *p; p++)
			{
				if (*p == '\r') *p = '\0';
				if (*p == '\n') *p = '\0';
			}

			ignore_exts.push_back(string(line));
		}

		fclose(file);

		printf("ignoring %u extensions\n", ignore_exts.size());

#ifdef _DEBUG
		for (size_t i = 0; i < ignore_exts.size(); i++)
		{
			printf("%s\n", ignore_exts.at(i).c_str());
		}
#endif
	}
	else
	{
		printf("couldn't open ignore file %s\n", ignore_file_name);
	}
}

static bool32 ext_in_ignore_list(const char *path)
{
	const char *ext = strrchr(path, '.');

	if (ext)
	{
		ext++;

		for (size_t i = 0; i < ignore_exts.size(); i++)
		{
			if (ignore_exts.at(i) == ext)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

static void fill_files(const char *search_dir)
{
	WIN32_FIND_DATA fd;
	string search_path = search_dir + string("\\*.*");
	HANDLE h = FindFirstFile(search_path.c_str(), &fd);

	if (h == INVALID_HANDLE_VALUE)
	{
		printf("the input path couldn't be found: %s\n", search_dir);
		exit(1);
	}

	do
	{
		if (!strequal(fd.cFileName, ".") && !strequal(fd.cFileName, ".."))
		{
			string full_path = string(search_dir) + "\\" + fd.cFileName;

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				fill_files(full_path.c_str());
			}
			else
			{
				if (!ext_in_ignore_list(full_path.c_str()))
				{
					files.push_back(full_path);
				}
			}
		}
	}
	while (FindNextFile(h, &fd));

	FindClose(h);
}

static bool32 is_directory(char *path)
{
	WIN32_FIND_DATA fd;
	HANDLE h = FindFirstFile(path, &fd);

	if (h != INVALID_HANDLE_VALUE)
	{
		FindClose(h);

		return fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	}

	printf("the input path couldn't be found: %s\n", path);
	exit(1);

	return FALSE;
}

static char *compact_exe_args;
static size_t compact_exe_args_alloc;

enum PROG_ARGS
{
	ARG_PROG_NAME = 0,
	ARG_PROG_MODE,
	ARG_FILE_OR_FOLDER,
	NUM_ARGS,
};

int32 main(int32 argc, char **argv)
{
	if (argc < NUM_ARGS)
	{
		return 1;
	}

	bool32 compacting;

	switch (*argv[ARG_PROG_MODE])
	{
	case 'c':
		compacting = TRUE;
		break;

	case 'u':
		compacting = FALSE;
		break;

	default:
		return 1;
	}

	fill_ignore_exts("compactxf.txt");

	if (is_directory(argv[ARG_FILE_OR_FOLDER]))
	{
		fill_files(argv[ARG_FILE_OR_FOLDER]);
	}

	if (!files.size() && !ext_in_ignore_list(argv[ARG_FILE_OR_FOLDER]))
	{
		files.push_back(argv[ARG_FILE_OR_FOLDER]);
	}

	printf("%s %u files\n", compacting ? "compacting" : "uncompacting", files.size());
	printf("\n");

	string compact_default_args = string("compact.exe") + " " + (compacting ? "/Q /C /I /EXE:XPRESS16K" : "/Q /U /I /EXE");

	for (size_t i = 0; i < files.size(); i++)
	{
		puts(files.at(i).c_str());

		string compact_exe_args_tmp = compact_default_args + " " + files.at(i);

		if (compact_exe_args_alloc < compact_exe_args_tmp.length())
		{
			free(compact_exe_args);

			compact_exe_args_alloc = compact_exe_args_tmp.length() + 2048;
			compact_exe_args = (char *)calloc(compact_exe_args_alloc, 1);
		}

		strcpy(compact_exe_args, compact_exe_args_tmp.c_str());

		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));

		si.cb = sizeof(si);
		si.dwFlags |= STARTF_USESTDHANDLES;
		si.hStdInput = NULL;
		si.hStdError = NULL;
		si.hStdOutput = NULL;

		if (CreateProcess(NULL, compact_exe_args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, INFINITE);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}

	free(compact_exe_args);
	compact_exe_args_alloc = 0;

	return 0;
}

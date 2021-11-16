#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

#define strequal(_Str1, _Str2) !strcmp(_Str1, _Str2)
#define striequal(_Str1, _Str2) !stricmp(_Str1, _Str2)

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

vector<string> ignore_exts;
vector<string> files;

enum PROG_MODE
{
	COMPACTING = 0,
	UNCOMPACTING,
	UNCOMPACTING_IGNORE_LIST,
};

static int32 mode = -1;

static void fill_ignore_exts()
{
	char module_path[MAX_PATH];
	GetModuleFileName(NULL, module_path, sizeof(module_path));

	char *ext = strrchr(module_path, '.');
	strcpy(ext + 1, "txt");

	FILE *file = fopen(module_path, "r");

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

#ifdef _DEBUG
		printf("ignoring %u extensions\n", ignore_exts.size());

		for (size_t i = 0; i < ignore_exts.size(); i++)
		{
			printf("%s\n", ignore_exts.at(i).c_str());
		}
#endif
	}
	else
	{
		printf("couldn't open ignore file %s\n", module_path);
	}
}

static const char *get_ext(const char *path)
{
	const char *ext = strrchr(path, '.');

	if (ext)
	{
		ext++;

		return ext;
	}

	return "";
}

static bool32 ext_in_ignore_list(const char *ext)
{
	for (auto it = ignore_exts.begin(); it != ignore_exts.end(); it++)
	{
		if (striequal(it->c_str(), ext))
		{
			return TRUE;
		}
	}

	return FALSE;
}

static bool32 ext_in_files(string &ext_path)
{
	for (auto it = files.rbegin(); it != files.rend(); it++)
	{
		if (striequal(it->c_str(), ext_path.c_str()))
		{
			return TRUE;
		}
	}

	return FALSE;
}

static bool32 path_contains_git(const char *path)
{
	return strstr(path, "\\.git\\") != NULL;
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
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				string full_path = string(search_dir) + "\\" + fd.cFileName;

				fill_files(full_path.c_str());
			}
			else
			{
				const char *ext = get_ext(fd.cFileName);

				if (mode == UNCOMPACTING_IGNORE_LIST)
				{
					if (*ext && ext_in_ignore_list(ext))
					{
						string ext_path = string(search_dir) + "\\*." + ext;

						if (!ext_in_files(ext_path))
						{
							files.push_back(ext_path);
						}
					}
					else
					{
						if (path_contains_git(search_dir))
						{
							string full_path = string(search_dir) + "\\" + fd.cFileName;

							files.push_back(full_path);
						}
					}
				}
				else
				{
					if (!path_contains_git(search_dir))
					{
						if (*ext)
						{
							if (!ext_in_ignore_list(ext))
							{
								string ext_path = string(search_dir) + "\\*." + ext;

								if (!ext_in_files(ext_path))
								{
									files.push_back(ext_path);
								}
							}
						}
						else
						{
							string full_path = string(search_dir) + "\\" + fd.cFileName;

							files.push_back(full_path);
						}
					}
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

static void create_process(char *cmd_line)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));

	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdInput = NULL;
	si.hStdError = NULL;
	si.hStdOutput = NULL;

	if (CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

enum PROG_ARGS
{
	ARG_PROG_NAME = 0,
	ARG_PROG_MODE,
	ARG_FILE_OR_FOLDER,
	NUM_ARGS,
};

#define USAGE \
"usage: compactxf c/u/i file/folder\n" \
"c for compacting, u for uncompacting, i for uncompacting files in ignore list\n"

int32 main(int32 argc, char **argv)
{
	if (argc < NUM_ARGS)
	{
		printf(USAGE);

		return 1;
	}

	switch (*argv[ARG_PROG_MODE])
	{
	case 'c':
		mode = COMPACTING;
		break;

	case 'u':
		mode = UNCOMPACTING;
		break;

	case 'i':
		mode = UNCOMPACTING_IGNORE_LIST;
		break;

	default:
		printf(USAGE);
		return 1;
	}

	fill_ignore_exts();

	if (is_directory(argv[ARG_FILE_OR_FOLDER]))
	{
		fill_files(argv[ARG_FILE_OR_FOLDER]);
	}
	else if (!ext_in_ignore_list(get_ext(argv[ARG_FILE_OR_FOLDER])))
	{
		files.push_back(string(argv[ARG_FILE_OR_FOLDER]));
	}

	char *compact_exe_args = NULL;
	size_t compact_exe_args_alloc = 0;
	string compact_default_args = string("compact.exe") + " " + (mode == COMPACTING ? "/Q /C /I /EXE:XPRESS16K" : "/Q /U /I /EXE");

	for (auto it = files.begin(); it != files.end(); it++)
	{
		puts(it->c_str());

		string compact_exe_args_tmp = compact_default_args + " " + *it;

		if (compact_exe_args_alloc < compact_exe_args_tmp.length())
		{
			free(compact_exe_args);

			compact_exe_args_alloc = compact_exe_args_tmp.length() + 2048;
			compact_exe_args = (char *)calloc(compact_exe_args_alloc, 1);
		}

		strcpy(compact_exe_args, compact_exe_args_tmp.c_str());

		create_process(compact_exe_args);
	}

	free(compact_exe_args);

	return 0;
}

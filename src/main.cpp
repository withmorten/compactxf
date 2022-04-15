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

static void fill_ignore_exts(vector<string> &ignore_exts)
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
		printf("ignoring %zu extensions\n", ignore_exts.size());

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

static bool ext_in_ignore_list(vector<string> &ignore_exts, const char *ext)
{
	for (size_t i = 0; i < ignore_exts.size(); i++)
	{
		if (striequal(ignore_exts.at(i).c_str(), ext))
		{
			return true;
		}
	}

	return false;
}

static bool ext_in_list(vector<string> &files, size_t start, string &ext_path)
{
	for (size_t i = start; i < files.size(); i++)
	{
		if (striequal(files.at(i).c_str(), ext_path.c_str()))
		{
			return true;
		}
	}

	return false;
}

static bool path_contains_git(const char *path)
{
	return strstr(path, "\\.git\\") != NULL;
}

static void fill_files(vector<string> &files, vector<string> &ignore_exts, const char *search_dir, bool only_ignore)
{
	static size_t start = 0;

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

				if (files.size()) start = files.size() - 1;

				fill_files(files, ignore_exts, full_path.c_str(), only_ignore);
			}
			else
			{
				const char *ext = get_ext(fd.cFileName);

				if (!path_contains_git(search_dir))
				{
					if (*ext)
					{
						if ((!only_ignore && !ext_in_ignore_list(ignore_exts, ext)) || (only_ignore && ext_in_ignore_list(ignore_exts, ext)))
						{
							string ext_path = string(search_dir) + "\\*." + ext;

							if (!ext_in_list(files, start, ext_path))
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
	while (FindNextFile(h, &fd));

	FindClose(h);
}

static bool is_directory(char *path)
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

	return false;
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

static void process_files(vector<string> &files, string &compact_default_args, char *&compact_exe_args, size_t &compact_exe_args_alloc)
{
	for (size_t i = 0; i < files.size(); i++)
	{
		string compact_exe_args_tmp = compact_default_args + " " + files.at(i);

		if (compact_exe_args_alloc < compact_exe_args_tmp.length())
		{
			free(compact_exe_args);

			compact_exe_args_alloc = compact_exe_args_tmp.length() + 1 + 2048;
			compact_exe_args = (char *)calloc(compact_exe_args_alloc, 1);
		}

		strcpy(compact_exe_args, compact_exe_args_tmp.c_str());

		puts(files.at(i).c_str());

		create_process(compact_exe_args);
	}
}

enum
{
	ARG_PROG_NAME = 0,
	ARG_PROG_MODE,
	ARG_FILE_OR_FOLDER,
	NUM_ARGS,
	ARG_OPT_COMP_MODE = NUM_ARGS
};

enum MODE
{
	COMPACTING,
	UNCOMPACTING,
	UNCOMPACTING_IGNORE,
};

#define USAGE \
"usage: compactxf c/u/i file/folder [4/8/16/lzx]\n" \
"c for compacting, u for uncompacting, i for uncompacting files in ignore list\n" \
"4 = XPRESS4K, 8 = XPRESS8K, 16 = XPRESS16K, lzx = LZX (ignored when uncompacting)\n"

int main(int argc, char **argv)
{
	if (argc < NUM_ARGS)
	{
		printf(USAGE);

		return 1;
	}

	MODE mode;

	switch (*argv[ARG_PROG_MODE])
	{
	case 'c':
		mode = COMPACTING;
		break;

	case 'u':
		mode = UNCOMPACTING;
		break;

	case 'i':
		mode = UNCOMPACTING_IGNORE;
		break;

	default:
		printf(USAGE);
		return 1;
	}

	char *compact_exe_args = NULL;
	size_t compact_exe_args_alloc = 0;

	vector<string> ignore_exts;
	vector<string> files;

	if (mode == COMPACTING)
	{
		string level = "XPRESS16K";

		if (argc > ARG_OPT_COMP_MODE)
		{
			if (strequal(argv[ARG_OPT_COMP_MODE], "4"))
			{
				level = "XPRESS4K";
			}
			else if (strequal(argv[ARG_OPT_COMP_MODE], "8"))
			{
				level = "XPRESS8K";
			}
			else if (strequal(argv[ARG_OPT_COMP_MODE], "16"))
			{
				level = "XPRESS16K";
			}
			else if (strequal(argv[ARG_OPT_COMP_MODE], "lzx"))
			{
				level = "LZX";
			}
			else
			{
				printf(USAGE);
				return 1;
			}
		}

#ifdef _DEBUG
		printf("compressing with %s\n", level.c_str());
#endif

		fill_ignore_exts(ignore_exts);

		if (is_directory(argv[ARG_FILE_OR_FOLDER]))
		{
			fill_files(files, ignore_exts, argv[ARG_FILE_OR_FOLDER], false);
		}
		else if (!ext_in_ignore_list(ignore_exts, get_ext(argv[ARG_FILE_OR_FOLDER])))
		{
			files.push_back(string(argv[ARG_FILE_OR_FOLDER]));
		}

		string compact_default_args = string("compact.exe") + " " + "/Q /C /I /EXE:" + level;

		process_files(files, compact_default_args, compact_exe_args, compact_exe_args_alloc);
	}
	else if (mode == UNCOMPACTING)
	{
		bool is_dir = is_directory(argv[ARG_FILE_OR_FOLDER]);

		string path = string(argv[ARG_FILE_OR_FOLDER]) + (is_dir ? "\\*.*" : "");

		string uncompact_args_tmp = string("compact.exe") + " " + (is_dir ? "/S " : "") + "/Q /U /I /EXE" + " " + path;

		compact_exe_args_alloc = uncompact_args_tmp.length() + 1;
		compact_exe_args = (char *)calloc(compact_exe_args_alloc, 1);

		strcpy(compact_exe_args, uncompact_args_tmp.c_str());

		puts(path.c_str());

		create_process(compact_exe_args);
	}
	else // mode == UNCOMPACTING_IGNORE
	{
		fill_ignore_exts(ignore_exts);

		if (is_directory(argv[ARG_FILE_OR_FOLDER]))
		{
			fill_files(files, ignore_exts, argv[ARG_FILE_OR_FOLDER], true);
		}
		else if (ext_in_ignore_list(ignore_exts, get_ext(argv[ARG_FILE_OR_FOLDER])))
		{
			files.push_back(string(argv[ARG_FILE_OR_FOLDER]));
		}

		string compact_default_args = string("compact.exe") + " " + "/Q /U /I";

		process_files(files, compact_default_args, compact_exe_args, compact_exe_args_alloc);
	}

	free(compact_exe_args);

	return 0;
}

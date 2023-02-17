#pragma once
#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <numeric>

using namespace std;
using namespace std::filesystem;

static const char Strings[] =
	"0123456789"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz";

inline string randstr(int len) {
	char* str = new char[(size_t)len + 1] { 0 };

	for (int i = 0; i < len; ++i)
		str[i] = Strings[rand() % (sizeof(Strings) - 1)];

	return str;
}

inline bool tempdir(path& dir, string prefix) {
	while (1) {
		dir = temp_directory_path() / path(prefix + randstr(8));
		if (!exists(dir)) break;
	}

	return create_directories(dir);
}

inline path curdir() {
	wchar_t filepath[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, filepath, MAX_PATH);
	return path(filepath).parent_path();
}

inline string strjoin(vector<string> strings, string delim) {
	if (strings.empty())
		return "";

	return accumulate(strings.begin() + 1, strings.end(), strings[0],
		[&delim](std::string x, std::string y) {
			return x + delim + y;
		}
	);
}

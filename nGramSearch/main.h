#pragma once

#include "stdafx.h"

#define DLLEXP extern "C" __declspec(dllexport)

static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(static_cast<unsigned char>(ch));
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(static_cast<unsigned char>(ch));
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
	ltrim(s);
	return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
	rtrim(s);
	return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
	trim(s);
	return s;
}

void toUpper(std::string& str)
{
	for (auto& ch : str)
		ch = toupper(ch);
}

inline bool compareScores(const std::pair<const std::string*, float>& a, const std::pair<const std::string*, float>& b)
{
	if (a.second > b.second)
		return true;
	if (a.second < b.second)
		return false;
	return a.first < b.first;
}

namespace std {
	class StringPointerHash {
	public:
		size_t operator() (const string *val) const {
			return std::hash<std::string>()(*val);
		}
	};

	class StringPointerEqual {
	public:
		bool operator()(const string *val1, const string *val2) const {
			return *val1 == *val2;
		}
	};
}


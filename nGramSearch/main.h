#pragma once

#include "stdafx.h"

#define DLLEXP extern "C" __declspec(dllexport)

static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(static_cast<unsigned char>(ch));
	}));
}

static inline void ltrimW(std::wstring &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
		return !std::iswspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(static_cast<unsigned char>(ch));
	}).base(), s.end());
}

static inline void rtrimW(std::wstring &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
		return !std::iswspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

static inline void trimW(std::wstring &s) {
	ltrimW(s);
	rtrimW(s);
}	 

static inline void toUpper(std::string& str)
{
	for (auto& ch : str)
		ch = toupper(ch);
}

static inline void toUpperW(std::wstring& str)
{
	for (auto& ch : str)
		ch = std::towupper(ch);
}

static inline bool compareScores(const std::pair<const std::string*, float>& a, const std::pair<const std::string*, float>& b)
{
	if (a.second > b.second)
		return true;
	if (a.second < b.second)
		return false;
	return a.first < b.first;
}

static inline bool compareScoresW(const std::pair<const std::wstring*, float>& a, const std::pair<const std::wstring*, float>& b)
{
	if (a.second > b.second)
		return true;
	if (a.second < b.second)
		return false;
	return a.first < b.first;
}

namespace std {
	struct StringPointerHash {
	public:
		size_t operator() (const string *val) const {
			return std::hash<std::string>()(*val);
		}
	};

	struct StringPointerHashW {
	public:
		size_t operator() (const wstring *val) const {
			return std::hash<std::wstring>()(*val);
		}
	};

	struct StringPointerEqual {
	public:
		bool operator()(const string *val1, const string *val2) const {
			return *val1 == *val2;
		}
	};

	struct StringPointerEqualW {
	public:
		bool operator()(const wstring *val1, const wstring *val2) const {
			return *val1 == *val2;
		}
	};
}

void getGrams(const std::string* str);
void getGrams(const std::string& str, std::vector<std::string>& generatedGrams);
void getGrams(const char* str, const int size, std::vector<std::string>& generatedGrams);
void buildGrams();
int stringMatch(const std::string& query, const std::string& source);
std::pair<const std::string*, int> getMatchScore(const std::string& query, const std::string* source);
void searchShort(std::string& query, std::unordered_map<const std::string*, float, std::StringPointerHash, std::StringPointerEqual>& score);
void searchLong(std::string& query, std::unordered_map<const std::string*, float, std::StringPointerHash, std::StringPointerEqual>& score);
void insert(std::vector<std::string>& key, std::vector<std::string>* additional = NULL, const int16_t gSize = 3);
DLLEXP bool insert(char** const key, const size_t size, char** const additional = NULL, const uint16_t gSize = 3);
void search(const char* query, const float threshold, const uint32_t limit, std::vector<const std::string*>& result);
DLLEXP bool search(const char* query, char*** results, uint32_t* nStrings, const float threshold = 0, uint32_t limit = 100);
DLLEXP void release(char*** results, size_t nStrings);
DLLEXP void clear();

void getGramsW(const std::wstring* str);
void getGramsW(const std::wstring& str, std::vector<std::wstring>& generatedGrams);
void getGramsW(const wchar_t* str, const int size, std::vector<std::wstring>& generatedGrams);
void buildGramsW();
int stringMatchW(const std::wstring& query, const std::wstring& source);
std::pair<const std::wstring*, int> getMatchScoreW(const std::wstring& query, const std::wstring* source);
void searchShortW(std::wstring& query, std::unordered_map<const std::wstring*, float, std::StringPointerHashW, std::StringPointerEqualW>& score);
void searchLongW(std::wstring& query, std::unordered_map<const std::wstring*, float, std::StringPointerHashW, std::StringPointerEqualW>& score);
void insertW(std::vector<std::wstring>& key, std::vector<std::wstring>* additional = NULL, const int16_t gSize = 3);
DLLEXP bool insertW(wchar_t** const key, const size_t size, wchar_t** const additional = NULL, const uint16_t gSize = 3);
void searchW(const wchar_t* query, const float threshold, const uint32_t limit, std::vector<const std::wstring*>& result);
DLLEXP bool searchW(const wchar_t* query, wchar_t*** results, uint32_t* nStrings, const float threshold = 0, uint32_t limit = 100);
DLLEXP void releaseW(wchar_t*** results, size_t nStrings);
DLLEXP void clearW();


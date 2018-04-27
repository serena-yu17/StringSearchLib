#pragma once
#ifndef NGRAMSEARCH_H
#define NGRAMSEARCH_H

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include <string>
#include <unordered_map>
#include <mutex>
#include <cctype>
#include <wchar.h>
#include <cwctype>
#include <unordered_set>
#include <algorithm>
#include <future>
#include <intrin.h>
#include <shared_mutex>
#include <memory>
#include <mutex>

#define DLLEXP extern "C" __declspec(dllexport)


inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(static_cast<unsigned char>(ch));
	}));
}

inline void ltrim(std::wstring &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
		return !std::iswspace(ch);
	}));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(static_cast<unsigned char>(ch));
	}).base(), s.end());
}

inline void rtrim(std::wstring &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
		return !std::iswspace(ch);
	}).base(), s.end());
}

inline void toUpper(std::string& str)
{
	for (char& ch : str)
		ch = toupper(ch);
}

inline void toUpper(std::wstring& str)
{
	for (wchar_t& ch : str)
		ch = towupper(ch);
}

template<class str_t>
class StringIndex
{
public:
	typedef typename str_t::value_type char_t;

	StringIndex(char_t** const key, const size_t size, char_t** const additional, const uint16_t gSize, float* weight = NULL);
	StringIndex(char_t*** const key, const size_t size, const uint16_t gSize, float* weight = NULL);
	StringIndex(std::vector<str_t>& key, std::vector<str_t>& additional, const int16_t gSize, float* weight = NULL);

	StringIndex(StringIndex<str_t>&& other)
	{
		longLib = std::move(other.longLib);
		shortLib = std::move(other.shortLib);
		wordMap = std::move(other.wordMap);
		ngrams = std::move(other.ngrams); 		
		weightVal = std::move(other.weightVal);
		gramSize = other.gramSize;
	}
	void init(std::unordered_map<str_t, std::pair<str_t, float>>& tempWordMap);
	void getGrams(str_t* str);
	void getGrams(const str_t& str, std::vector<str_t>& generatedGrams);
	//void getGrams(const char_t* str, const int size, std::vector<str_t>& generatedGrams);
	void buildGrams();
	size_t stringMatch(const str_t& query, const str_t& source, std::vector<size_t>& row1, std::vector<size_t>& row2);
	void getMatchScore(const str_t& query, size_t lb, std::vector<str_t*>& targets, std::vector<float>& currentScore);
	void searchShort(str_t& query, std::unordered_map<str_t*, float>& score);
	void searchLong(str_t& query, std::unordered_map<str_t*, float>& score);	
	void search(const char_t* query, const float threshold, const uint32_t limit, std::vector<str_t>& result);
	void search(const char_t* query, char_t*** results, uint32_t* nStrings, const float threshold = 0, uint32_t limit = 100);
	void release(char_t*** results, size_t nStrings);
	void buildHash();
	void getHash(str_t* str);

	StringIndex& operator=(StringIndex<str_t>&& other)
	{
		if (this != &other)
		{
			longLib = std::move(other.longLib);
			shortLib = std::move(other.shortLib);
			wordMap = std::move(other.wordMap);
			ngrams = std::move(other.ngrams);
			weightVal = std::move(other.weightVal);
			gramSize = other.gramSize;
		}
	}

	template<class str_t>
	static inline bool compareScores(std::pair<str_t*, float>& a, std::pair<str_t*, float>& b)
	{
		if (a.second > b.second)
			return true;
		if (a.second < b.second)
			return false;
		return *(a.first) < *(b.first);
	}

	// trim from both ends (in place)
	template<class str_t>
	static inline void trim(str_t &s) {
		ltrim(s);
		rtrim(s);
	}

protected:
	std::vector<str_t> longLib;
	std::vector<str_t> shortLib;
	std::unordered_map<str_t*, std::pair<str_t, float>> wordMap;
	std::unordered_map<str_t, std::vector<const str_t*>> ngrams;
	std::unordered_map<str_t, std::vector<const str_t*>> approxHash;
	int16_t gramSize = 3;

private:
	int sectionSize = 3000;
};

DLLEXP void index2D(char* const guid, char*** const key, const size_t size, const uint16_t gSize = 3, float* weight = NULL);
DLLEXP void index2DW(char* const guid, wchar_t*** const key, const size_t size, const uint16_t gSize = 3, float* weight = NULL);
DLLEXP void index(char* const guid, char** const key, const size_t size, char** const additional = NULL, const uint16_t gSize = 3, float* weight = NULL);
DLLEXP void indexW(char* const guid, wchar_t** const key, const size_t size, wchar_t** const additional = NULL, const uint16_t gSize = 3, float* weight = NULL);
DLLEXP void search(char* const guid, const char* query, char*** results, uint32_t* nStrings, const float threshold = 0, uint32_t limit = 100);
DLLEXP void searchW(char* const guid, const wchar_t* query, wchar_t*** results, uint32_t* nStrings, const float threshold = 0, uint32_t limit = 100);
DLLEXP void release(char* const guid, char*** results, size_t nStrings);
DLLEXP void releaseW(char* const guid, wchar_t*** results, size_t nStrings);
DLLEXP void dispose(char* const guid);
DLLEXP void disposeW(char* const guid);

#endif


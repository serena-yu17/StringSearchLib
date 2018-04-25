#pragma once

#include "stdafx.h"

#define DLLEXP extern "C" __declspec(dllexport)

class StringIndex 
{
public:
	void getGrams(std::string* str);
	void getGrams(const std::string& str, std::vector<std::string>& generatedGrams);
	void getGrams(const char* str, const int size, std::vector<std::string>& generatedGrams);
	void buildGrams();
	int stringMatch(const std::string& query, const std::string& source);
	std::pair<std::string*, int> getMatchScore(const std::string& query, std::string* source);
	void searchShort(std::string& query, std::unordered_map<std::string, float>& score);
	void searchLong(std::string& query, std::unordered_map<std::string, float>& score);
	void insert(std::vector<std::string>& key, std::vector<std::string>* additional = NULL, const int16_t gSize = 3);
	void insert(char** const key, const size_t size, char** const additional = NULL, const uint16_t gSize = 3);
	void search(const char* query, const float threshold, const uint32_t limit, std::vector<std::string>& result);
	void search(const char* query, char*** results, uint32_t* nStrings, const float threshold = 0, uint32_t limit = 100);
	void release(char*** results, size_t nStrings);

private:
	std::vector<std::string> longLib;
	std::vector<std::string> shortLib;
	std::unordered_map<const std::string*, std::string> wordMap;
	std::unordered_map<std::string, std::vector<const std::string*>> ngrams;
	int16_t gramSize = 3;

	std::mutex mutScore;

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

	static inline void toUpper(std::string& str)
	{
		for (auto& ch : str)
			ch = toupper(ch);
	}

	static inline bool compareScores(std::pair<std::string, float>& a, std::pair<std::string, float>& b)
	{
		if (a.second > b.second)
			return true;
		if (a.second < b.second)
			return false;
		return a.first < b.first;
	}
};


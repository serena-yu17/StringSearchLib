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
#include <atomic>
#include <cmath>

#define DLLEXP extern "C" __declspec(dllexport)


/*
Index the library based on a 2D array.
@param key For each row, the first string is the key to be mapped to, and the second string is the description mapped to the key
@param guid A unique id for the indexed library
@param size size of the [key]
@param gSize size of grams to be created. Default 3
@param weight A list of the relative weight of each key. Default 1 for all
*/
DLLEXP void index2D(char* const guid, char*** const key, const uint64_t size, const uint16_t rowSize = 1, float** const weight = NULL,
	char*** const attr = NULL, const uint16_t attrRowSize = 0, uint64_t* priceRange = NULL, const uint16_t gSize = 3);

/*@{
Index the library based on a 2D array. Wide string version
@param key For each row, the first string is the key to be mapped to, and the second string is the description mapped to the key
@param guid A unique id for the indexed library
@param size size of the [key]
@param gSize size of grams to be created. Default 3
@param weight A list of the relative weight of each key. Default 1 for all
@}*/
DLLEXP void index2DW(char* const guid, wchar_t*** const key, const uint64_t size, const uint16_t rowSize = 1, float** const weight = NULL,
	wchar_t*** const attr = NULL, const uint16_t attrRowSize = 0, uint64_t* priceRange = NULL, const uint16_t gSize = 3);

/*@{
Index the library based on a string array of key, and another array of additional text, e.g. description.
Finally the additional will be mapped back to the keys.
@param guid A unique id for the indexed library
@param key keys to be searched for
@param size size of the array for keys
@param rowSize size of each additional text rows
@param gSize size of grams to be created. Default 3
@param weight A list of weight values for each key. It should be at least as long as the key array.
@}*/
DLLEXP void index(char* const guid, char** const key, const uint64_t size, const uint16_t rowSize = 1, float* const weight = NULL,
	char** const attr = NULL, const uint16_t attrRowSize = 0, uint64_t* priceRange = NULL, const uint16_t gSize = 3);

/*@{
Wide string version to index the library based on a string array of key, and another array of additional text, e.g. description.
Finally the additional will be mapped back to the keys.
@param guid A unique id for the indexed library
@param key keys to be searched for
@param size size of the array for keys
@param rowSize size of each additional text rows
@param gSize size of grams to be created. Default 3
@param weight A list of weight values for each key. It should be at least as long as the key array.
@}*/
DLLEXP void indexW(char* const guid, wchar_t** const key, const uint64_t size, const uint16_t rowSize, float* const weight = NULL,
	wchar_t** const attr = NULL, const uint16_t attrRowSize = 0, uint64_t* priceRange = NULL, const uint16_t gSize = 3);

/*@{
search the query in the indexed library identified by the guid.
@param guid A unique id for the indexed library
@param query The query string
@param results The pointer to a string array for output. The memory will be allocated by new
@param nStrings Output the length of the results array.
@param threshold lowest acceptable matching%, as a value between 0 and 1
@param limit Maximum results generated, default 100
@}*/
DLLEXP void search(char* const guid, char* const query, char** const filters, int64_t* const filterSize, uint64_t* priceRange, char*** results, uint32_t* nStrings, const float threshold, const uint32_t limit);

/*@{
A wide string version to search the query in the indexed library identified by the guid.
@param guid A unique id for the indexed library
@param query The query string
@param results The pointer to a string array for output. The memory will be allocated by new
@param nStrings Output the length of the results array.
@param threshold lowest acceptable matching%, as a value between 0 and 1
@param limit Maximum results generated, default 100
@}*/
DLLEXP void searchW(char* const guid, wchar_t* const query, wchar_t** const filters, int64_t* const filterSize, uint64_t* priceRange, wchar_t*** results, uint32_t* nStrings, const float threshold, const uint32_t limit);

/*@{
To release the memory allocated for the result in the <search> function
@param guid A unique id for the indexed library
@param results The result returned by the <search> function.
@param nStrings Length of <result>
@}*/
DLLEXP void release(char* const guid, char*** results, uint64_t nStrings);

/*@{
To release the memory allocated for the result in the <search> function. Wide string version.
@param guid A unique id for the indexed library
@param results The result returned by the <search> function.
@param nStrings Length of <result>
@}*/
DLLEXP void releaseW(char* const guid, wchar_t*** results, uint64_t nStrings);

/*@{
To dispose a library indexed. If the library does not exist, <dispose> will ignore it.
@param guid A unique id for the indexed library
@}*/
DLLEXP void dispose(char* const guid);

/*@{
Wide string version. To dispose a library indexed. If the library does not exist, <dispose> will ignore it.
@param guid A unique id for the indexed library
@}*/
DLLEXP void disposeW(char* const guid);

/*@{
To obtain the current word map size
@param guid A unique id for the indexed library
@}*/
DLLEXP uint64_t getSize(char* const guid);

/*@{
Wide string version. To obtain the current word map size
@param guid A unique id for the indexed library
@}*/
DLLEXP uint64_t getLibSize(char* const guid);

/*@{
To obtain the current gram library size
@param guid A unique id for the indexed library
@}*/
DLLEXP uint64_t getSizeW(char* const guid);

/*@{
Wide string version. To obtain the current gram library size
@param guid A unique id for the indexed library
@}*/
DLLEXP uint64_t getLibSizeW(char* const guid);

namespace
{
	//Allowed words for the query string. Other characters in the ASCII range will be converted to spaces
	const std::unordered_set<char> wordChar
	({
		'.','%','$',' ',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
		});

	/*@{
	Trims spaces in place from left of string
	@param s String to be trimmed
	@}*/
	inline void ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(static_cast<unsigned char>(ch));
		}));
	}

	/*@{
	Trims spaces in place from left of string, wide string version
	@param s String to be trimmed
	@}*/
	inline void ltrim(std::wstring &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
			return !std::iswspace(ch);
		}));
	}

	/*@{
	Trims spaces in place from right of string
	@param s String to be trimmed
	@}*/
	inline void rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(static_cast<unsigned char>(ch));
		}).base(), s.end());
	}

	/*@{
	Trims spaces in place from right of string, wide string version
	@param s String to be trimmed
	@}*/
	inline void rtrim(std::wstring &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
			return !std::iswspace(ch);
		}).base(), s.end());
	}

	/*@{
	Converts the string to upper case in place
	@param str String to be converted
	@}*/
	inline void toUpper(std::string& str)
	{
		for (char& ch : str)
			ch = toupper(ch);
	}

	/*@{
	Converts the string to upper case in place, wide string version
	@param str String to be converted
	@}*/
	inline void toUpper(std::wstring& str)
	{
		for (wchar_t& ch : str)
			ch = towupper(ch);
	}

	/*@{
	Escapes all invalid characters to spaces
	@param str String to be converted
	@}*/
	inline void escapeBlank(std::string& str)
	{
		for (char& ch : str)
			if (wordChar.find(ch) == wordChar.end())
				ch = ' ';
	}

	/*@{
	Escapes all invalid characters to spaces, wide string version. Only characters that fit into the ASCII range will be converted
	@param str String to be converted
	@}*/
	inline void escapeBlank(std::wstring& str)
	{
		for (wchar_t& ch : str)
		{
			if (ch >= 0 && ch <= 127)
			{
				char asciiChar = (char)ch;
				if (wordChar.find(asciiChar) == wordChar.end())
					ch = ' ';
			}
		}
	}
}

/*@{
StringIndex: Each instance manages a library from the <index> function
@param str_t A STL string type. Can be std::string or std::wstring
@}*/
template<class str_t>
class StringIndex
{
public:
	//The character type invelved in the str_t
	typedef typename str_t::value_type char_t;

	/*@{

	@}*/
	StringIndex(char_t** const key, const size_t size, const uint16_t rowSize, float* const weight, char_t** const attr, const uint16_t attrRowSize, size_t* priceRange, const uint16_t gSize);
	StringIndex(char_t*** const key, const size_t size, const uint16_t rowSize, float** const weight, char_t*** const attr, const uint16_t attrRowSize, size_t* priceRange, const uint16_t gSize);
	StringIndex(std::vector<std::vector<str_t>>& key, std::vector<std::vector<float>>& weight, std::vector<std::vector<str_t>>& attr, std::vector<std::vector<size_t>>& priceRange, const int16_t gSize);

	StringIndex(StringIndex<str_t>&& other)
	{
		longLib = std::move(other.longLib);
		shortLib = std::move(other.shortLib);
		wordMap = std::move(other.wordMap);
		ngrams = std::move(other.ngrams);
		weightVal = std::move(other.weightVal);
		gramSize = other.gramSize;
	}

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

	void execSearch(char_t* const query, char_t** const filters, int64_t* const filterSize, size_t* priceRange, const float threshold, const uint32_t limit, std::vector<str_t>& result);
	void search(char_t* const query, char_t** const filters, int64_t* const filterSize, size_t* priceRange, char_t*** results, uint32_t* nStrings, const float threshold = 0, const uint32_t limit = 100);
	void release(char_t*** results, size_t nStrings) const;
	uint64_t size();
	uint64_t libSize();

	template<class str_t>
	static inline bool compareScores(std::pair<str_t, float>& a, std::pair<str_t, float>& b)
	{
		if (a.second > b.second)
			return true;
		if (a.second < b.second)
			return false;
		return a.first.size() < b.first.size();
	}


	template<class str_t>
	static inline bool compareInt(std::pair<str_t, int>& a, std::pair<str_t, int>& b)
	{
		return a.second > b.second;
	}

	// trim from both ends (in place)
	template<class str_t>
	static inline void trim(str_t &s)
	{
		ltrim(s);
		rtrim(s);
	}

protected:
	std::vector<str_t> longLib;
	std::vector<str_t> shortLib;
	std::vector<str_t> keyLib;
	std::unordered_map<str_t*, std::vector<str_t*>> wordMap;
	std::unordered_map<str_t*, float> wordScore;
	std::unordered_map<str_t*, std::vector<str_t>> keyAttr;
	std::unordered_map<str_t*, std::pair<size_t, size_t>> keyPrice;
	std::unordered_map<str_t, std::unordered_set<str_t*>> ngrams;
	//std::unordered_map<str_t, std::vector<str_t*>> approxHash;
	int16_t gramSize = 3;

private:
	size_t sectionSize = 1000;
	std::atomic<bool> indexed = false;
	const float distanceFactor = 0.2f;
	uint16_t attrLength = 0;
	void filterScore(std::unordered_map<str_t*, float>& keyPair, std::unordered_map<str_t, float>& entryScore,
		std::vector<std::unordered_set<str_t>>& filterVec, std::vector<bool>& filterStatus, size_t* priceRange, float threshold);
	bool fitAttribute(std::vector<str_t>& attr, std::vector<std::unordered_set<str_t>>& filterVec, std::vector<bool>& filterStatus);
	void init(std::unordered_map<str_t, std::vector<str_t>>& tempWordMap, std::unordered_map<str_t, float>& tempKeyScore,
		std::unordered_map<str_t, std::vector<str_t>>& tempAttrLst, std::unordered_map<str_t, std::pair<size_t, size_t>>& tempKeyPrice);
	void getGrams(str_t* str);
	void getGrams(const str_t& str, std::vector<str_t>& generatedGrams);
	//void getGrams(const char_t* str, const int size, std::vector<str_t>& generatedGrams);
	void buildGrams();
	size_t stringMatch(const str_t& query, const str_t& source, std::vector<size_t>& row1, std::vector<size_t>& row2);
	void getMatchScore(const str_t& query, size_t lb, std::vector<str_t*>& targets, std::vector<float>& currentScore);
	void searchShort(str_t& query, std::unordered_map<str_t*, float>& score);
	void searchLong(str_t& query, std::unordered_map<str_t*, float>& score);
};

#endif


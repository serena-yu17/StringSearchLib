#pragma once
#ifndef NGRAMSEARCH_H
#define NGRAMSEARCH_H

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

namespace
{
	//!Allowed words for the query string. Other characters in the ASCII range will be converted to spaces
	const std::unordered_set<char> wordChar
	({
		'.','%','$',' ', '@',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
	});

	/*!
	Trims spaces in place from left of string
	@param s String to be trimmed
	*/
	inline void ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(static_cast<unsigned char>(ch));
		}));
	}

	/*!
	Trims spaces in place from left of string.
	Wide string version.
	@param s String to be trimmed
	*/
	inline void ltrim(std::wstring &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
			return !std::iswspace(ch);
		}));
	}

	/*!
	Trims spaces in place from right of string
	@param s String to be trimmed
	*/
	inline void rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(static_cast<unsigned char>(ch));
		}).base(), s.end());
	}

	/*!
	Trims spaces in place from right of string.
	Wide string version.
	@param s String to be trimmed
	*/
	inline void rtrim(std::wstring &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
			return !std::iswspace(ch);
		}).base(), s.end());
	}

	/*!
	Converts the string to upper case in place
	@param str String to be converted
	*/
	inline void toUpper(std::string& str)
	{
		for (char& ch : str)
			ch = toupper(ch);
	}

	/*!
	Converts the string to upper case in place. 
	Wide string version.
	@param str String to be converted
	*/
	inline void toUpper(std::wstring& str)
	{
		for (wchar_t& ch : str)
			ch = towupper(ch);
	} 

	/*!
	Escapes all invalid characters to spaces
	@param str String to be converted 
	*/
	inline void escapeBlank(std::string& str)
	{
		for (char& ch : str)
			if (wordChar.find(ch) == wordChar.end())
				ch = ' ';
	}

	/*!
	Escapes all invalid characters to spaces. Only characters that fit into the ASCII range will be converted.
	Wide string version.
	@param str String to be converted
	*/
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

/*!
StringIndex: Each instance manages a library from the <index> function
@param str_t A STL string type. Can be std::string or std::wstring
*/
template<class str_t>
class StringIndex
{
public:
	//! The character type contained in the str_t
	using char_t = typename str_t::value_type;

	/*!
	Constructs the StringIndex class by indexing the strings based on an array of words
	@param words Words to be searched for. For each row, the first word is used as the master key, in which the row size is \p rowSize. 
	All rows are flattened into a 1D-array, and can be extracted based on \p rowSize.
	In a search, all queries of the words in a row will return the master key.
	@param size size of the \p words
	@param rowSize size of each text rows of \p words.
	@param weight A list of weight values for each key. It should be at least as long as the number of rows, i.e. \p size / \p rowSize.
	@param gSize size of grams to be created. Default 3.
	*/
	StringIndex(char_t** const words, const size_t size, const uint16_t rowSize, float* const weight, const uint16_t gSize);

	/*!
	Constructs the StringIndex class by indexing the strings based on an array of words
	@param words Words to be searched for. For each row, the first word is used as the master key, in which the row size is \p rowSize.
	Each row is in a separate sub-array. In a search, all queries of the words in a row will return the master key.
	@param size size of the \p words
	@param rowSize size of each text rows of \p words.
	@param weight A list of weight values for each key. It should be at least as long as the number of rows, i.e. \p size / \p rowSize.
	@param gSize size of grams to be created. Default 3.
	*/
	StringIndex(char_t*** const words, const size_t size, const uint16_t rowSize, float** const weight, const uint16_t gSize);

	/*!
	Constructs the StringIndex class by indexing the strings based on an array of words
	@param words Words to be searched for. For each row, the first word is used as the master key, in which the row size is \p rowSize.
	Each row is in a separate sub-array. In a search, all queries of the words in a row will return the master key.
	@param size size of the \p words
	@param rowSize size of each text rows of \p words.
	@param weight A list of weight values for each key. It should be at least as long as the number of rows, i.e. \p size / \p rowSize.
	@param gSize size of grams to be created. Default 3.
	*/
	StringIndex(std::vector<std::vector<str_t>>& words, const int16_t gSize, std::vector<std::vector<float>>& weight);		

	/*!
	Initiates the word map by assigning the same strings to a pointer, to save space.
	@param tempWordMap A temprary word map of strings. 
	Key: query terms. Value: a list of master keys and corresponding scores that the queries point to.
	*/
	void init(std::unordered_map<str_t, std::vector<str_t>>& tempWordMap, std::unordered_map<str_t, std::unordered_map<str_t, float>>& tempKeyScore);

	/*!
	Generate n-grams from a string based on the member variable \p gramSize.
	@param str A pointer to the string to generate n-grams from.
	*/
	void getGrams(size_t id);

	/*!
	Generate n-grams from a string based on the member variable \p gramSize, and store in an array.
	@param str A pointer to the string to generate n-grams from.
	@param generatedGrams A vector to store the genearated n-grams
	*/
	void getGrams(const str_t& str, std::vector<str_t>& generatedGrams);

	/*!
	Build n-grams for the member variable \p longLib
	*/
	void buildGrams();

	/*!
	Computes the percentage of \p query matches \p source. 
	@param query A query string
	@param source A source string in the library to compare to.
	@param row1 A temporary vector as a cache for the algorithm. Its size must at least (the max size of \p query and \p source) + 1.
	@param row2 A temporary vector as a cache for the algorithm. Its size must at least (the max size of \p query and \p source) + 1.
	*/
	size_t stringMatch(const str_t& query, const str_t& source, std::vector<size_t>& row1, std::vector<size_t>& row2);

	/*!
	A looper to calculate match scores
	@param query The query string.
	@param first The starting index to loop from.
	@param targets The target strings that have been scored
	@param currentScore The score for each strings in \p targets
	*/
	void getMatchScore(const str_t& query, size_t first, std::vector<size_t>& targets, std::vector<float>& currentScore); 

	/*!
	Search in the shortLib
	@param query The query string.
	@param score Targets found paired with their corresponding cores generated.
	*/
	void searchShort(str_t& query, std::unordered_map<size_t, float>& score);

	/*!
	Search in the longLib
	@param query The query string.
	@param score Targets found paired with their corresponding cores generated.
	*/
	void searchLong(str_t& query, std::unordered_map<size_t, float>& score);

	void calcScore(std::unordered_map<size_t, float>& entryScore, std::unordered_map<size_t, float>& scoreList, const float threshold);

	/*!
	The worker function for search
	@param query The query string.
	@param threshold Lowest acceptable match ratio for a string to be included in the results.
	@param limit The maximum number of results to generate.
	@param result The matching strings to be selected, sorted from highest score to lowest.
	*/
	void _search(const char_t* query, const float threshold, const uint32_t limit, std::vector<size_t>& result);

	/*!
	The search interface function, calls \p _search
	@param query The query string.
	@param results The matching strings to be selected, sorted from highest score to lowest.
	@param size The number of strings in the result array.
	@param threshold Lowest acceptable match ratio for a string to be included in the results.
	@param limit The maximum number of results to generate.	
	*/
	void search(const char_t* query, char_t*** results, uint32_t* size, const float threshold, uint32_t limit);

	/*!
	Release a result pointer that have been generated in \p search
	@param results The matching strings to be selected, allocated using the \p new operator.
	*/
	void release(char_t*** results, size_t size) const;

	/*!
	Get the size of the word map \p wordMap
	*/
	uint64_t size();

	/*!
	Get the size of the n-gram library \p ngrams
	*/
	uint64_t libSize();	

	/*!
	Trim a string from both ends (in place)
	@param s The string to be trimmed
	*/
	template<class str_t>
	static inline void trim(str_t &s) 
	{
		ltrim(s);
		rtrim(s);
	}


	template<class str_t>
	struct ScoreComparer
	{
	public:
		const std::vector<str_t>& stringLib;

		ScoreComparer(const StringIndex<str_t>& instance) : stringLib(instance.stringLib)
		{ }

		/*!
		Compare pairs of string-score by their score, and length. Greater scores and shorter lengths will be prioritized
		@param a The first pair of string-score
		@param b The second pair of string-score
		*/
		bool operator()(const std::pair<size_t, float>& a, const std::pair<size_t, float>& b) const
		{
			if (a.second > b.second)
				return true;
			if (a.second < b.second)
				return false;
			return stringLib[a.first].size() < stringLib[b.first].size();
		}
	};

private:
	std::vector<str_t> stringLib;

	//! The library for all words that have a length >= \p gramSize * 2
	std::vector<size_t> longLib;

	//! The library for all words that have a length < \p gramSize * 2
	std::vector<size_t> shortLib;

	//! All words, mapped to their master keys. A search result will always be redirected to its master keys
	std::unordered_map<size_t, std::vector<size_t>> wordMap;

	//! Weights to keys
	std::unordered_map<size_t, std::unordered_map<size_t, float>> wordWeight;

	//! The n-gram library generated
	std::unordered_map<str_t, std::unordered_set<size_t>> ngrams;

	//! the size of n-grams
	int16_t gramSize = 3;

	//! The section size of strings for each \p getMatchScore loop block
	size_t sectionSize = 1000;

	//! Indicator of whether the library has been indexed. If not indexed, no search can be done.
	std::atomic<bool> indexed = false;

	//! deprecated
	const float distanceFactor = 0.2f;
};


#endif


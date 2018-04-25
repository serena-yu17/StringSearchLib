// nGramSearch.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "nGramSearch.h"
using namespace std;

DLLEXP StringIndex<string>* index(char*** const key, const size_t size, const uint16_t gSize = 3)
{
	return new StringIndex<string>(key, size, gSize);
}

DLLEXP StringIndex<string>* index(char** const key, const size_t size, char** const additional = NULL, const uint16_t gSize = 3)
{
	return new StringIndex<string>(key, size, additional, gSize);
}

DLLEXP StringIndex<wstring>* indexW(wchar_t** const key, const size_t size, wchar_t** const additional = NULL, const uint16_t gSize = 3)
{
	return new StringIndex<wstring>(key, size, additional, gSize);
}

DLLEXP void release(StringIndex<string>* si, char*** results, size_t nStrings)
{
	si->release(results, nStrings);
}

DLLEXP void releaseW(StringIndex<wstring>* si, wchar_t*** results, size_t nStrings)
{
	si->release(results, nStrings);
}

DLLEXP void dispose(StringIndex<string>* si)
{
	delete si;
}

DLLEXP void disposeW(StringIndex<wstring>* si)
{
	delete si;
}


template<class str_t>
void StringIndex<str_t>::getGrams(str_t* str)
{
	for (size_t i = 0; i < str->size() - gramSize + 1; i++)
	{
		auto gram = str->substr(i, gramSize);
		ngrams[gram].push_back(str);
	}
}

template<class str_t>
void StringIndex<str_t>::getGrams(const str_t& str, vector<str_t>& generatedGrams)
{
	for (int i = 0; i < str.size() - gramSize + 1; i++)
	{
		auto gram = str.substr(i, gramSize);
		generatedGrams.push_back(gram);
	}
}

template<class str_t>
void StringIndex<str_t>::getGrams(const char_t* str, const int size, vector<str_t>& generatedGrams)
{
	for (int i = 0; i < size - gramSize + 1; i++)
	{
		str_t gram;
		for (int j = i; j < i + gramSize; j++)
			gram.push_back(str[j]);
		generatedGrams.push_back(gram);
	}
}

template<class str_t>
void StringIndex<str_t>::buildGrams()
{
	for (str_t& str : longLib)
		getGrams(&str); 
}

template<class str_t>
int StringIndex<str_t>::stringMatch(const str_t& query, const str_t& source)
{
	if (query.size() == 1)
	{
		for (auto ch : source)
			if (ch == query[0])
				return 1;
		return 0;
	}

	unsigned qSize = (unsigned)query.size();
	unsigned sSize = (unsigned)source.size();
	auto maxSize = max(qSize, sSize);
	vector<unsigned> row1(maxSize + 1, 0);
	vector<unsigned> row2(maxSize + 1, 0);
	for (unsigned q = 0; q < qSize; q++)
	{
		row2[0] = q + 1;
		for (unsigned s = 0; s < sSize; s++)
		{
			int cost = 0;
			if (query[q] != source[s])
				cost = 1;
			row2[s + 1] =
				min(
					min(
						row1[s + 1] + 1,
						row2[s] + 1),
					row1[s] + cost);
		}
		swap(row1, row2);
		//memset(row2, 0, sizeof(unsigned) * (maxSize + 1));
		fill(row2.begin(), row2.end(), 0);
	}
	unsigned misMatch = (numeric_limits<unsigned>::max)();
	for (unsigned i = 0; i < sSize + 1; i++)
		if (row1[i] < misMatch)
			misMatch = row1[i];
	return qSize - misMatch;
}

template<class str_t>
pair<str_t*, int> StringIndex<str_t>::getMatchScore(const str_t& query, str_t* source)
{
	int match = stringMatch(query, *source);
	return make_pair(source, match);
}

template<class str_t>
void StringIndex<str_t>::searchShort(str_t& query, unordered_map<str_t, float>& score)
{
	unsigned len = (unsigned)query.size();
	vector<future<pair<str_t*, int>>> futures;
	for (auto str : shortLib)
		futures.emplace_back(async(std::launch::async, &StringIndex::getMatchScore, this, query, &str));
	for (int i = 0; i < futures.size(); i++)
	{
		auto res = futures[i].get();
		std::lock_guard<mutex> lock(mutScore);
		if (score.find(*res.first) == score.end())
			score[*res.first] = (float)res.second / len;
		else
			score[*res.first] += (float)res.second / len;
	}
}

template<class str_t>
void StringIndex<str_t>::searchLong(str_t& query, unordered_map<str_t, float>& score)
{
	auto len = query.size();
	if (len < gramSize)
		return;

	vector<str_t> generatedGrams;
	getGrams(query, generatedGrams);
	if (generatedGrams.size() == 0)
		return;
	unordered_map<str_t, size_t> rawScore;
	for (auto& str : generatedGrams)
	{
		const auto& sourceSet = ngrams[str];
		for (auto match : sourceSet)
			rawScore[*match] ++;
	}
	for (auto& keyPair : rawScore)
	{
		std::lock_guard<mutex> lock(mutScore);
		score[keyPair.first] += (float)keyPair.second / generatedGrams.size();
	}
}

template<class str_t>
void StringIndex<str_t>::insert(vector<str_t>& key, vector<str_t>* additional, const int16_t gSize)
{
	std::unordered_map<str_t, str_t> tempWordMap;
	if (gSize < 2 || key.size() < 2)
		return;
	for (size_t i = 0; i < key.size(); i++)
	{
		str_t strKey(key[i]);
		trim(strKey);
		str_t upperKey(strKey);
		toUpper(upperKey);
		tempWordMap[upperKey] = strKey;

		if (additional)
		{
			str_t strQuery((*additional)[i]);
			trim(strQuery);
			toUpper(strQuery);
			tempWordMap[strQuery] = strKey;
		}
	}
	gramSize = gSize;
	for (auto& keyPair : tempWordMap)
	{
		auto& str = keyPair.first;
		if (str.size() >= gramSize * 2)
		{
			longLib.push_back(str);
			wordMap[&longLib.back()] = keyPair.second;
		}
		else
		{
			shortLib.push_back(str);
			wordMap[&shortLib.back()] = keyPair.second;
		}
	}
	buildGrams();
}

template<class str_t>
StringIndex<str_t>::StringIndex(char_t** const key, const size_t size, char_t** const additional, const uint16_t gSize)
{  	
	std::unordered_map<str_t, str_t> tempWordMap;
	if (gSize < 2 || size < 2)
		return;
	for (size_t i = 0; i < size; i++)
	{
		str_t strKey(key[i]);
		trim(strKey);
		str_t upperKey(strKey);
		toUpper(upperKey);
		tempWordMap[upperKey] = strKey;

		if (additional)
		{
			str_t strQuery(additional[i]);
			trim(strQuery);
			toUpper(strQuery);
			tempWordMap[strQuery] = strKey;
		}
	}
	gramSize = gSize;
	for (auto& keyPair : tempWordMap)
	{
		auto& str = keyPair.first;
		if (str.size() >= (size_t)gramSize * 2)
		{
			longLib.push_back(str);
			wordMap[&longLib.back()] = keyPair.second;
		}
		else
		{
			shortLib.push_back(str);
			wordMap[&shortLib.back()] = keyPair.second;
		}
	}
	buildGrams();
}

template<class str_t>
StringIndex<str_t>::StringIndex(char_t*** const key, const size_t size, const uint16_t gSize)
{
	std::unordered_map<str_t, str_t> tempWordMap;
	if (gSize < 2 || size < 2)
		return;
	for (size_t i = 0; i < size; i++)
	{
		str_t strKey(key[i][0]);
		trim(strKey);
		str_t upperKey(strKey);
		toUpper(upperKey);
		tempWordMap[upperKey] = strKey;

		str_t strQuery(key[i][1]);
		trim(strQuery);
		toUpper(strQuery);
		tempWordMap[strQuery] = strKey;
	}
	gramSize = gSize;
	for (auto& keyPair : tempWordMap)
	{
		auto& str = keyPair.first;
		if (str.size() >= (size_t)gramSize * 2)
		{
			longLib.push_back(str);
			wordMap[&longLib.back()] = keyPair.second;
		}
		else
		{
			shortLib.push_back(str);
			wordMap[&shortLib.back()] = keyPair.second;
		}
	}
	buildGrams();
}

template<class str_t>
void StringIndex<str_t>::search(const char_t* query, const float threshold, const uint32_t limit, vector<str_t>& result)
{
	str_t queryStr(query);
	toUpper(queryStr);
	unordered_map<str_t, float> score;
	vector<future<void>> futures;
	futures.emplace_back(
		std::async(std::launch::async, &StringIndex::searchShort, this, std::ref(queryStr), ref(score))
	);
	futures.emplace_back(
		std::async(std::launch::async, &StringIndex::searchLong, this, std::ref(queryStr), ref(score))
	);
	for (auto& fu : futures)
		fu.get();

	//merge
	/*for (auto& keyPair : scoreLong)
		scoreShort[keyPair.first] += keyPair.second;*/
	vector<pair<str_t, float>> scoreElems(score.begin(), score.end());	 
	sort(scoreElems.begin(), scoreElems.end(), compareScores);

	for (size_t i = 0; i < scoreElems.size() && i < limit && scoreElems[i].second >= threshold; i++)
		result.push_back(scoreElems[i].first);
}

template<class str_t>
void StringIndex<str_t>::search(const char_t* query, char_t*** results, uint32_t* nStrings, const float threshold, uint32_t limit)
{
	if (wordMap.size() == 0 || gramSize == 0)
		return;

	if (limit == 0)
		limit = (numeric_limits<int32_t>::max)();

	vector<str_t> result;

	search(query, threshold, limit, result);

	//transform to C ABI using pointers
	*nStrings = (uint32_t)result.size();
	*results = new char_t*[*nStrings];
	for (uint32_t i = 0; i < *nStrings; i++)
	{
		auto item = result[i];
		auto resStr = item.c_str();
		auto len = item.size();
		(*results)[i] = new char_t[len + 1]();
		memcpy((*results)[i], resStr, len);
	}
}

template<class str_t>
void StringIndex<str_t>::release(char_t*** results, size_t nStrings)
{
	if (*results)
	{
		for (size_t i = 0; i < nStrings; i++)
			delete[](*results)[i];
		delete[](*results);
	}
}
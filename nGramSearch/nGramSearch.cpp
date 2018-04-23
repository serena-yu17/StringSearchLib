// nGramSearch.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "main.h"
using namespace std;

vector<const string*> longLib;
vector<const string*> shortLib;
unordered_map<string, string> wordMap;
unordered_map<string, unordered_set<const string*>> ngrams;
int16_t gramSize = 3;

void getGrams(const string& const str, vector<string>* generatedGrams)
{
	for (int i = 0; i < str.size() - gramSize + 1; i++)
	{
		auto gram = str.substr(i, gramSize);
		generatedGrams->push_back(gram);
	}
}

void getGrams(const char* str, const int size, vector<string>* generatedGrams)
{
	for (int i = 0; i < size - gramSize + 1; i++)
	{
		string gram;
		for (int j = i; j < i + gramSize; j++)
			gram.push_back(str[j]);
		generatedGrams->push_back(gram);
	}
}

void buildGrams()
{
	for (auto& str : longLib)
	{
		vector<string> gneratedGrams;
		getGrams(*str, &gneratedGrams);
		for (auto& gram : gneratedGrams)
		{
			if (ngrams.find(gram) == ngrams.end())
				ngrams[gram] = unordered_set<const string*>();
			ngrams[gram].insert(str);
		}
	}
}

int stringMatch(const string& query, const string& source)
{
	if (query.size() == 1)
	{
		for (auto ch : source)
			if (ch == query[0])
				return 1;
		return 0;
	}
	auto qSize = query.size();
	auto sSize = source.size();
	auto maxSize = max(qSize, sSize);
	int* row1 = new	int[maxSize + 1]();
	int* row2 = new	int[maxSize + 1]();
	for (size_t q = 0; q < qSize; q++)
	{
		row2[0] = q + 1;
		for (size_t s = 0; s < sSize; s++)
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
		memset(row2, 0, sizeof(int) * (maxSize + 1));
	}
	int misMatch = (numeric_limits<int>::max)();
	for (auto i = 0; i < sSize; i++)
		if (row1[i] < misMatch)
			misMatch = row1[i];
	delete[] row1;
	delete[] row2;
	return qSize - misMatch;
}

pair<const string*, int> getMatchScore(const string& query, const string* source)
{
	int match = stringMatch(query, *source);
	return make_pair(source, match);
}

void searchShort(string& const query, unordered_map<const string*, float>* score)
{
	int len = query.size();
	vector<future<pair<const string*, int>>> futures;
	for (const string* str : shortLib)
		futures.emplace_back(async(std::launch::async, getMatchScore, query, str));
	for (int i = 0; i < futures.size(); i++)
	{
		auto res = futures[i].get();
		if (score->find(res.first) == score->end())
			(*score)[res.first] = res.second / len;
		else
			(*score)[res.first] += res.second / len;
	}
}

void searchLong(string& const query, unordered_map<const string*, float>* score)
{
	int len = query.size();
	if (len < gramSize)
		return;

	vector<string> generatedGrams;
	getGrams(query, &generatedGrams);
	if (generatedGrams.size() == 0)
		return;
	unordered_map<const string*, size_t> rawScore;
	for (auto& str : generatedGrams)
	{
		const auto& sourceSet = ngrams[str];
		for (auto match : sourceSet)
		{
			if (rawScore.find(match) == rawScore.end())
				rawScore[match] = 1;
			else
				rawScore[match] ++;
		}
	}
	for (auto& keyPair : rawScore)
		if (score->find(keyPair.first) == score->end())
			(*score)[keyPair.first] = keyPair.second / generatedGrams.size();
		else
			(*score)[keyPair.first] += keyPair.second / generatedGrams.size();
}

void insert(vector<string>& key, vector<string>* query = NULL, const int16_t gSize = 3)
{
	if (gSize < 2 || key.size() < 2)
		return;
	for (size_t i = 0; i < key.size(); i++)
	{
		string strKey(key[i]);
		trim(strKey);
		string upperKey(strKey);
		toUpper(upperKey);
		wordMap[upperKey] = strKey;

		if (query)
		{
			string strQuery((*query)[i]);
			trim(strQuery);
			toUpper(strQuery);
			wordMap[strQuery] = strKey;
		}
	}
	gramSize = gSize;
	for (auto& keyPair : wordMap)
	{
		auto& str = keyPair.first;
		if (str.size() >= gramSize * 2)
			longLib.push_back(&str);
		else
			shortLib.push_back(&str);
	}
	buildGrams();
}

DLLEXP bool insert(char** const key, const size_t size, char** const query = NULL, const int16_t gSize = 3)
{
	if (gSize < 2 || size < 2)
		return false;
	for (size_t i = 0; i < size; i++)
	{
		string strKey(key[i]);
		trim(strKey);
		string upperKey(strKey);
		toUpper(upperKey);
		wordMap[upperKey] = strKey;

		if (query)
		{
			string strQuery(query[i]);
			trim(strQuery);
			toUpper(strQuery);
			wordMap[strQuery] = strKey;
		}
	}
	gramSize = gSize;
	for (auto& keyPair : wordMap)
	{
		auto& str = keyPair.first;
		if (str.size() >= gramSize * 2)
			longLib.push_back(&str);
		else
			shortLib.push_back(&str);
	}
	buildGrams();
	return true;
}

void search(const char* query, const float threshold, const size_t limit, vector<const string*>* result)
{
	string queryStr(query);
	toUpper(queryStr);
	unordered_map<const string*, float> score;
	searchShort(queryStr, &score);
	searchLong(queryStr, &score);
	vector<pair<const string*, float>> scoreElems(score.begin(), score.end());
	sort(scoreElems.begin(), scoreElems.end(),
		[](const pair<const string*, float>& a, const pair<const string*, float>& b)
	{
		return a.second > b.second;
	});
	for (size_t i = 0; i < scoreElems.size() && i < limit && scoreElems[i].second >= threshold; i++)
		result->push_back(scoreElems[i].first);
}

DLLEXP bool search(const char* query, const float threshold, const size_t limit, char*** results, int16_t* nStrings)
{
	if (wordMap.size() == 0 || gramSize == 0)
		return false;

	vector<const string*> result;

	search(query, threshold, limit, &result);

	*nStrings = result.size();
	*results = new char*[*nStrings];
	for (int i = 0; i < *nStrings; i++)
	{
		auto item = result[i];
		auto resStr = item->c_str();
		auto len = item->size();
		(*results)[i] = new char[len + 1]();
		memcpy((*results)[i], resStr, len);
	}
	return true;
}

DLLEXP void releaseWord(char*** results, int nStrings)
{
	if (*results)
	{
		for (int i = 0; i < nStrings; i++)
			delete[](*results)[i];
		delete[](*results);
	}
}

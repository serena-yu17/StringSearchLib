// nGramSearch.cpp : Defines the exported functions for the DLL application.
//

#include "nGramSearch.h"
using namespace std;

shared_mutex mainLock;
//key entries for indexed StringIndex class instances
unordered_map<string, unique_ptr<StringIndex<string>>> indexed;
unordered_map<string, unique_ptr<StringIndex<wstring>>> indexedW;


DLLEXP void index2D(char* const guid, char*** const key, const uint64_t size, const uint16_t rowSize, const uint16_t gSize, float* const weight)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexed.emplace(string(guid), make_unique<StringIndex<string>>(key, (size_t)size, rowSize, gSize));
}

DLLEXP void index2DW(char* const guid, wchar_t*** const key, const uint64_t size, const uint16_t rowSize, const uint16_t gSize, float* const weight)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexedW.emplace(string(guid), make_unique<StringIndex<wstring>>(key, (size_t)size, rowSize, gSize));
}

DLLEXP void index(char* const guid, char** const key, const uint64_t size, char** const additional, const uint16_t rowSize,
	const uint16_t gSize, float* const weight)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexed.emplace(string(guid), make_unique<StringIndex<string>>(key, (size_t)size, additional, rowSize, gSize, weight));
}

DLLEXP void indexW(char* const guid, wchar_t** const key, const uint64_t size, wchar_t** const additional, const uint16_t rowSize,
	const uint16_t gSize, float* const weight)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexedW.emplace(string(guid), make_unique<StringIndex<wstring>>(key, (size_t)size, additional, rowSize, gSize, weight));
}

DLLEXP void search(char* const guid, const char* query, char*** results, uint32_t* nStrings, const float threshold, uint32_t limit)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto pkeyPair = indexed.find(string(guid));
	if (pkeyPair != indexed.end())
	{
		auto& instance = pkeyPair->second;
		//sharedLock.unlock();
		instance->search(query, results, nStrings, threshold, limit);
	}
}

DLLEXP void searchW(char* const guid, const wchar_t* query, wchar_t*** results, uint32_t* nStrings, const float threshold, uint32_t limit)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto pkeyPair = indexedW.find(string(guid));
	if (pkeyPair != indexedW.end())
	{
		auto& instance = pkeyPair->second;
		//sharedLock.unlock();
		instance->search(query, results, nStrings, threshold, limit);
	}
}

DLLEXP void release(char* const guid, char*** results, uint64_t nStrings)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		(instance->second)->release(results, (size_t)nStrings);
}

DLLEXP void releaseW(char* const guid, wchar_t*** results, uint64_t nStrings)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexedW.find(string(guid));
	if (instance != indexedW.end())
		(instance->second)->release(results, (size_t)nStrings);
}

DLLEXP void dispose(char* const guid)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexed.erase(string(guid));
}

DLLEXP void disposeW(char* const guid)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexedW.erase(string(guid));
}

DLLEXP uint64_t getSize(char* const guid)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		return instance->second->size();
	return 0;
}

DLLEXP uint64_t getLibSize(char* const guid)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		return instance->second->libSize();
	return 0;
}

DLLEXP uint64_t getSizeW(char* const guid)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexedW.find(string(guid));
	if (instance != indexedW.end())
		return instance->second->size();
	return 0;
}

DLLEXP uint64_t getLibSizeW(char* const guid)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexedW.find(string(guid));
	if (instance != indexedW.end())
		return instance->second->libSize();
	return 0;
}


template<class str_t>
void StringIndex<str_t>::getGrams(str_t* str)
{
	for (size_t i = 0; i < str->size() - gramSize + 1; i++)
	{
		auto gram = str->substr(i, gramSize);
		ngrams[gram].emplace_back(make_pair(str, i));
	}
}

template<class str_t>
void StringIndex<str_t>::getGrams(const str_t& str, std::vector<str_t>& generatedGrams)
{
	for (size_t i = 0; i < str.size() - gramSize + 1; i++)
		generatedGrams.emplace_back(str.substr(i, gramSize));
}

template<class str_t>
void StringIndex<str_t>::buildGrams()
{
	for (str_t& str : longLib)
		getGrams(&str);
	indexed = true;
}

template<class str_t>
void StringIndex<str_t>::init(std::unordered_map<str_t, std::vector<std::pair<str_t, float>>>& tempWordMap)
{
	wordMap.clear();
	shortLib.clear();
	longLib.clear();
	wordMap.reserve(tempWordMap.size());

	size_t len = 0;

	//to separate the long and short libs, since different algorithms will be applied upon searches
	for (auto& keyPair : tempWordMap)
	{
		auto& str = keyPair.first;
		if (str.size() >= (size_t)gramSize * 2)
			longLib.push_back(str);
		else
			shortLib.push_back(str);
	}
	//use pointers in wordMap instead of replicating the whole string to save space.
	//the pointers should be unique for each string with the above processing, so they make a good hash key
	for (auto& str : shortLib)
		wordMap[&str] = std::move(tempWordMap[str]);
	for (auto& str : longLib)
		wordMap[&str] = std::move(tempWordMap[str]);

	//Each future involves an overhead of about 1-2 us, so I am limiting the number of futures to hardware concurrency
	auto nThrd = std::thread::hardware_concurrency();
	size_t sizeNeeded = (size_t)ceil((float)shortLib.size() / nThrd);
	if (sizeNeeded > sectionSize)
		sectionSize = sizeNeeded;
}

template<class str_t>
StringIndex<str_t>::StringIndex(vector<str_t>& key, vector<vector<str_t>>& additional, const int16_t gSize, float* weight)
{
	if (gSize < 2 || key.size() < 2)
		return;
	std::unordered_map<str_t, pair<str_t, float>> tempWordMap(size);
	for (size_t i = 0; i < key.size(); i++)
	{
		str_t strKey(key[i]);
		trim(strKey);

		if (strKey.size() == 0)
			continue;
		str_t upperKey(strKey);
		toUpper(upperKey);

		float currentWeight = 0.0f;
		if (weight)
			currentWeight = weight[i];
		auto value = make_pair(strKey, currentWeight);

		tempWordMap[upperKey].push_back(value);

		if (additional.size() == key.size())
			for (size_t j = 0; j < additional[i].size(); j++)
			{
				str_t strQuery(additional[i][j]);
				trim(strQuery);
				if (strQuery.size() != 0)
				{
					toUpper(strQuery);
					tempWordMap[strQuery].push_back(value);
				}
			}
	}
	gramSize = gSize;
	init(tempWordMap);
	buildGrams();
}

template<class str_t>
StringIndex<str_t>::StringIndex(char_t** const key, const size_t size, char_t** const additional, const uint16_t rowSize, const uint16_t gSize, float* weight)
{
	if (gSize < 2 || size < 2)
		return;
	std::unordered_map<str_t, std::vector<std::pair<str_t, float>>> tempWordMap(size);
	for (size_t i = 0; i < size; i++)
	{
		str_t strKey(key[i]);
		trim(strKey);
		if (strKey.size() == 0)
			continue;
		str_t upperKey(strKey);
		escapeBlank(upperKey);
		trim(upperKey);
		toUpper(upperKey);

		float currentWeight = 0.0f;
		if (weight)
			currentWeight = weight[i];
		auto value = make_pair(strKey, currentWeight);

		tempWordMap[upperKey].push_back(value);

		if (additional)
			for (size_t j = rowSize * i; j < rowSize * i + rowSize; j++)
				if (additional[j])
				{
					str_t strQuery(additional[j]);
					escapeBlank(strQuery);
					trim(strQuery);
					toUpper(strQuery);
					if (strQuery.size() != 0)
						tempWordMap[strQuery].push_back(value);
				}
	}
	gramSize = gSize;
	init(tempWordMap);
	buildGrams();
}

template<class str_t>
StringIndex<str_t>::StringIndex(char_t*** const key, const size_t size, const uint16_t rowSize, const uint16_t gSize, float* weight)
{
	if (gSize < 2 || size < 2)
		return;
	std::unordered_map<str_t, std::vector<std::pair<str_t, float>>> tempWordMap(size);
	for (size_t i = 0; i < size; i++)
	{
		str_t strKey(key[i][0]);
		trim(strKey);
		if (strKey.size() == 0)
			continue;
		str_t upperKey(strKey);
		escapeBlank(upperKey);
		trim(upperKey);
		toUpper(upperKey);

		float currentWeight = 0.0f;
		if (weight)
			currentWeight = weight[i];
		auto value = make_pair(strKey, currentWeight);
		tempWordMap[upperKey].push_back(value);

		if (rowSize != 0)
			for (uint16_t j = 1; j < rowSize; j++)
			{
				str_t strQuery(key[i][j]);
				escapeBlank(strQuery);
				trim(strQuery);
				toUpper(strQuery);

				if (strQuery.size() != 0)
					tempWordMap[strQuery].push_back(value);
			}
	}
	gramSize = gSize;
	init(tempWordMap);
	buildGrams();
}

template<class str_t>
size_t StringIndex<str_t>::stringMatch(const str_t& query, const str_t& source, std::vector<size_t>& row1, std::vector<size_t>& row2)
{
	if (query.size() == 1)
	{
		for (auto ch : source)
			if (ch == query[0])
				return 1;
		return 0;
	}

	size_t qSize = query.size();
	size_t sSize = source.size();
	auto maxSize = max(qSize, sSize);

	fill(row1.begin(), row1.end(), 0);
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
		//memset(row2, 0, sizeof(unsigned) * (maxSize + 1));
		fill(row2.begin(), row2.end(), 0);
	}
	size_t misMatch = (numeric_limits<size_t>::max)();
	for (unsigned i = 0; i < sSize + 1; i++)
		if (row1[i] < misMatch)
			misMatch = row1[i];
	return qSize - misMatch;
}

template<class str_t>
void StringIndex<str_t>::getMatchScore(const str_t& query, size_t lb, std::vector<str_t*>& targets, std::vector<float>& currentScore)
{
	//allocate levenstein temporary containers
	size_t maxSize = max(query.size(), (size_t)gramSize * 2 + 1);
	vector<size_t> row1(maxSize + 1);
	vector<size_t> row2(maxSize + 1);
	for (size_t i = lb; i < lb + sectionSize && i < shortLib.size(); i++)
	{
		str_t& source = shortLib[i];
		auto match = stringMatch(query, source, row1, row2);
		currentScore[i] = (float)match / query.size();
		targets[i] = &source;
	}
}

template<class str_t>
void StringIndex<str_t>::searchShort(str_t& query, unordered_map<str_t*, float>& score)
{
	auto len = query.size();
	auto dicSize = shortLib.size();
	vector<str_t*> targets(dicSize);
	vector<float> currentScore(dicSize);
	vector<future<void>> futures;
	for (size_t count = 0; count < dicSize; count += sectionSize)
		futures.emplace_back(async(&StringIndex::getMatchScore, this, cref(query), count, ref(targets), ref(currentScore)));
	for (auto& fu : futures)
		fu.get();
	for (size_t i = 0; i < dicSize; i++)
		score[targets[i]] += currentScore[i];
}

//template<class str_t>
//void StringIndex<str_t>::searchShort(str_t& query, unordered_map<str_t*, float>& score)
//{
//	str_t signature = getHash(query); 
//}


template<class str_t>
void StringIndex<str_t>::searchLong(str_t& query, unordered_map<str_t*, float>& score)
{
	auto len = query.size();
	if (len < (size_t)gramSize)
		return;

	std::vector<str_t> generatedGrams;
	generatedGrams.reserve(len - gramSize + 1);
	getGrams(query, generatedGrams);
	if (generatedGrams.size() == 0)
		return;
	std::unordered_map<str_t*, std::vector<size_t>> rawScore(longLib.size());
	//may consider parallelsm here in the future
	for (auto& str : generatedGrams)
	{
		const auto& sourceSet = ngrams[str];
		for (auto& match : sourceSet)
			rawScore[const_cast<str_t*>(match.first)].push_back(match.second);
	}
	for (auto& kp : rawScore)
	{
		vector<size_t>& position = kp.second;
		size_t start = 0;
		size_t longest = 0, longestStart = 0, longestEnd = 0;
		//find longest consective sequence
		for (size_t i = 1; i < position.size(); i++)
		{
			if (position[i - 1] != position[i] - 1 || i == position.size() - 1)
			{
				if (i - start > longest)
				{
					longest = i - start;
					longestStart = start;
					longestEnd = i - 1;
				}
				start = i;
			}
		}
		float currentScore = 0;
		for (size_t i = 1; i < position.size(); i++)
		{
			auto distance = abs((long long)i - (long long)longestStart) - ((long long)position[i] - (long long)position[longestStart]);
			if (distance != 0)
				currentScore += 1.0f / distance;
			else
				currentScore += 1.0f;
		}
		score[kp.first] += currentScore;
	}
}

template<class str_t>
void StringIndex<str_t>::search(const char_t* query, const float threshold, const uint32_t limit, vector<str_t>& result)
{
	str_t queryStr(query);
	escapeBlank(queryStr);
	trim(queryStr);
	if (queryStr.size() == 0)
		return;
	toUpper(queryStr);
	unordered_map<str_t*, float> scoreShort(shortLib.size());
	unordered_map<str_t*, float> scoreLong(longLib.size());
	vector<future<void>> futures;
	//if the query is long, there is no need to search for short sequences.
	if (queryStr.size() < (size_t)gramSize * 4)
		futures.emplace_back(
			std::async(std::launch::async, &StringIndex::searchShort, this, std::ref(queryStr), ref(scoreShort))
		);
	futures.emplace_back(
		std::async(std::launch::async, &StringIndex::searchLong, this, std::ref(queryStr), ref(scoreLong))
	);
	for (auto& fu : futures)
		fu.get();

	//merge
	unordered_map<str_t, float> entryScore(scoreShort.size() + scoreLong.size());
	for (auto& keyPair : scoreShort)
	{
		if (keyPair.second < threshold)
			continue;
		str_t* term = keyPair.first;
		auto mapped = wordMap.find(term);
		if (mapped != wordMap.end())
			for (auto& item : (*mapped).second)
			{
				str_t res = item.first;
				float itemScore = item.second * keyPair.second;
				entryScore[res] = itemScore;
			}
	}
	for (auto& keyPair : scoreLong)
	{
		if (keyPair.second < threshold)
			continue;
		str_t* term = keyPair.first;
		auto mapped = wordMap.find(term);
		if (mapped != wordMap.end())
			for (auto& item : (*mapped).second)
			{
				str_t res = item.first;
				float itemScore = max(item.second * keyPair.second, entryScore[res]);
				entryScore[res] = itemScore;
			}
	}

	vector<pair<str_t, float>> scoreElems(entryScore.begin(), entryScore.end());
	auto endIt = scoreElems.end();
	if (scoreElems.size() > limit)
		endIt = scoreElems.begin() + limit;
	partial_sort(scoreElems.begin(), endIt, scoreElems.end(), compareScores<str_t>);

	result.reserve(min(limit, scoreElems.size()));
	for (auto i = scoreElems.begin(); i < endIt; i++)
		result.push_back((*i).first);
}

template<class str_t>
void StringIndex<str_t>::search(const char_t* query, char_t*** results, uint32_t* nStrings, const float threshold, uint32_t limit)
{
	if (!indexed)
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
void StringIndex<str_t>::release(char_t*** results, size_t nStrings) const
{
	if (*results)
	{
		for (size_t i = 0; i < nStrings; i++)
			delete[](*results)[i];
		delete[](*results);
	}
}

template<class str_t>
uint64_t StringIndex<str_t>::size()
{
	return wordMap.size();
}

template<class str_t>
uint64_t StringIndex<str_t>::libSize()
{
	return ngrams.size();
}

//template<class str_t>
//void StringIndex<str_t>::buildHash()
//{
//	for (auto& str : shortLib)
//		approxHash[&str] = getHash(str);;
//}
//
//template<class str_t>
//str_t StringIndex<str_t>::getHash(str_t& str)
//{
//	set<char_t> charSet;
//	for (auto ch : str)
//		charSet.insert(ch);
//	str_t signature(charSet.begin(), charSet.end());
//	return signature;
//}
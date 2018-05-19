// nGramSearch.cpp : Defines the exported functions for the DLL application.
//

#include "nGramSearch.h"
using namespace std;

shared_mutex mainLock;
//key entries for indexed StringIndex class instances
unordered_map<string, unique_ptr<StringIndex<string>>> indexed;
unordered_map<string, unique_ptr<StringIndex<wstring>>> indexedW;

/*@{
Index the library based on a 2D array.
@param key For each row, the first string is the key to be mapped to, and the second string is the description mapped to the key
@param guid A unique id for the indexed library
@param size size of the [key]
@param gSize size of grams to be created. Default 3
@param weight A list of the relative weight of each key. Default 1 for all
@}
*/
DLLEXP void index2D(char* const guid, char*** const key, const uint64_t size, const uint16_t rowSize, float** const weight, const uint16_t gSize)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexed.emplace(string(guid), make_unique<StringIndex<string>>(key, (size_t)size, rowSize, weight, gSize));
}

/*@{
Index the library based on a 2D array. Wide string version
@param key For each row, the first string is the key to be mapped to, and the second string is the description mapped to the key
@param guid A unique id for the indexed library
@param size size of the [key]
@param gSize size of grams to be created. Default 3
@param weight A list of the relative weight of each key. Default 1 for all
@}*/
DLLEXP void index2DW(char* const guid, wchar_t*** const key, const uint64_t size, const uint16_t rowSize, float** const weight, const uint16_t gSize)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexedW.emplace(string(guid), make_unique<StringIndex<wstring>>(key, (size_t)size, rowSize, weight, gSize));
}

/*@{
Index the library based on a string array of key, and another array of additional text, e.g. description.
Finally the additional will be mapped back to the keys.
@param guid A unique id for the indexed library
@param key keys to be searched for
@param size size of the array for keys
@param additional an array of additional text, e.g. descriptions. All rows must have a uniform length. If some strings are missing, leave as blank
@param rowSize size of each additional text rows
@param gSize size of grams to be created. Default 3
@param weight A list of weight values for each key. It should be at least as long as the key array.
@}*/
DLLEXP void index(char* const guid, char** const key, const uint64_t size, const uint16_t rowSize, float* const weight, const uint16_t gSize)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexed.emplace(string(guid), make_unique<StringIndex<string>>(key, (size_t)size, rowSize, weight, gSize));
}

/*@{
Wide string version to index the library based on a string array of key, and another array of additional text, e.g. description.
Finally the additional will be mapped back to the keys.
@param guid A unique id for the indexed library
@param key keys to be searched for
@param size size of the array for keys
@param additional an array of additional text, e.g. descriptions. All rows must have a uniform length. If some strings are missing, leave as blank
@param rowSize size of each additional text rows
@param gSize size of grams to be created. Default 3
@param weight A list of weight values for each key. It should be at least as long as the key array.
@}*/
DLLEXP void indexW(char* const guid, wchar_t** const key, const uint64_t size, const uint16_t rowSize, float* const weight, const uint16_t gSize)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexedW.emplace(string(guid), make_unique<StringIndex<wstring>>(key, (size_t)size, rowSize, weight, gSize));
}

/*@{
search the query in the indexed library identified by the guid.
@param guid A unique id for the indexed library
@param query The query string
@param results The pointer to a string array for output. The memory will be allocated by new
@param size Output the length of the results array.
@param threshold lowest acceptable matching%, as a value between 0 and 1
@param limit Maximum results generated, default 100
@}*/
DLLEXP void search(char* const guid, const char* query, char*** results, uint32_t* size, const float threshold, uint32_t limit)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto pkeyPair = indexed.find(string(guid));
	if (pkeyPair != indexed.end())
	{
		auto& instance = pkeyPair->second;
		//sharedLock.unlock();
		instance->search(query, results, size, threshold, limit);
	}
}

/*@{
A wide string version to search the query in the indexed library identified by the guid.
@param guid A unique id for the indexed library
@param query The query string
@param results The pointer to a string array for output. The memory will be allocated by new
@param size Output the length of the results array.
@param threshold lowest acceptable matching%, as a value between 0 and 1
@param limit Maximum results generated, default 100
@}*/
DLLEXP void searchW(char* const guid, const wchar_t* query, wchar_t*** results, uint32_t* size, const float threshold, uint32_t limit)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto pkeyPair = indexedW.find(string(guid));
	if (pkeyPair != indexedW.end())
	{
		auto& instance = pkeyPair->second;
		//sharedLock.unlock();
		instance->search(query, results, size, threshold, limit);
	}
}

/*@{
To release the memory allocated for the result in the <search> function
@param guid A unique id for the indexed library
@param results The result returned by the <search> function.
@param size Length of <result>
@}*/
DLLEXP void release(char* const guid, char*** results, uint64_t size)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		(instance->second)->release(results, (size_t)size);
}

/*@{
To release the memory allocated for the result in the <search> function. Wide string version.
@param guid A unique id for the indexed library
@param results The result returned by the <search> function.
@param size Length of <result>
@}*/
DLLEXP void releaseW(char* const guid, wchar_t*** results, uint64_t size)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexedW.find(string(guid));
	if (instance != indexedW.end())
		(instance->second)->release(results, (size_t)size);
}

/*@{
To dispose a library indexed. If the library does not exist, <dispose> will ignore it.
@param guid A unique id for the indexed library
@}*/
DLLEXP void dispose(char* const guid)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexed.erase(string(guid));
}

/*@{
Wide string version. To dispose a library indexed. If the library does not exist, <dispose> will ignore it.
@param guid A unique id for the indexed library
@}*/
DLLEXP void disposeW(char* const guid)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexedW.erase(string(guid));
}

/*@{
To obtain the current word map size
@param guid A unique id for the indexed library
@}*/
DLLEXP uint64_t getSize(char* const guid)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		return instance->second->size();
	return 0;
}

/*@{
Wide string version. To obtain the current word map size
@param guid A unique id for the indexed library
@}*/
DLLEXP uint64_t getLibSize(char* const guid)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		return instance->second->libSize();
	return 0;
}

/*@{
To obtain the current gram library size
@param guid A unique id for the indexed library
@}*/
DLLEXP uint64_t getSizeW(char* const guid)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto instance = indexedW.find(string(guid));
	if (instance != indexedW.end())
		return instance->second->size();
	return 0;
}

/*@{
Wide string version. To obtain the current gram library size
@param guid A unique id for the indexed library
@}*/
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
		ngrams[gram].insert(str);
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
StringIndex<str_t>::StringIndex(char_t** const key, const size_t size, const uint16_t rowSize, float* const weight, const uint16_t gSize)
{
	if (gSize < 2 || size < 2)
		return;
	std::unordered_map<str_t, std::vector<std::pair<str_t, float>>> tempWordMap(size);
	for (size_t i = 0; i < size; i += rowSize)
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
		tempWordMap[upperKey].emplace_back(make_pair(strKey, currentWeight));

		for (size_t j = i + 1; j < i + rowSize; j++)
			if (key[j])
			{
				str_t strQuery(key[j]);
				escapeBlank(strQuery);
				trim(strQuery);
				toUpper(strQuery);
				if (strQuery.size() != 0)
				{
					currentWeight = 0.0f;
					if (weight)
						currentWeight = weight[j];
					tempWordMap[strQuery].emplace_back(make_pair(strKey, currentWeight));
				}
			}
	}
	gramSize = gSize;
	init(tempWordMap);
	buildGrams();
}

template<class str_t>
StringIndex<str_t>::StringIndex(std::vector<std::vector<str_t>>& key, const int16_t gSize, std::vector<std::vector<float>>& weight)
{
	if (gSize < 2 || key.size() < 2)
		return;
	std::unordered_map<str_t, pair<str_t, float>> tempWordMap(size);
	for (size_t i = 0; i < key.size(); i++)
	{
		auto& row = key[i];
		str_t strKey(row[0]);
		trim(strKey);

		if (strKey.size() == 0)
			continue;
		str_t upperKey(strKey);
		toUpper(upperKey);

		float currentWeight = 0.0f;
		if (weight)
			currentWeight = weight[i][0];

		tempWordMap[upperKey].emplace_back(make_pair(strKey, currentWeight));

		for (size_t j = 1; j < row.size(); j++)
		{
			str_t strQuery(row[j]);
			trim(strQuery);
			if (strQuery.size() != 0)
			{
				currentWeight = 0.0f;
				if (weight)
					currentWeight = weight[i][j];
				toUpper(strQuery);
				tempWordMap[strQuery].emplace_back(make_pair(strKey, currentWeight));
			}
		}
	}
	gramSize = gSize;
	init(tempWordMap);
	buildGrams();
}

template<class str_t>
StringIndex<str_t>::StringIndex(char_t*** const key, const size_t size, const uint16_t rowSize, float** weight, const uint16_t gSize)
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
			currentWeight = weight[i][0];
		tempWordMap[upperKey].emplace_back(make_pair(strKey, currentWeight));

		for (uint16_t j = 1; j < rowSize; j++)
			if (key[i][j])
			{
				str_t strQuery(key[i][j]);
				escapeBlank(strQuery);
				trim(strQuery);
				toUpper(strQuery);
				if (strQuery.size() != 0)
				{
					currentWeight = 0.0f;
					if (weight)
						currentWeight = weight[i][j];
					toUpper(strQuery);
					tempWordMap[strQuery].emplace_back(make_pair(strKey, currentWeight));
				}
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
	std::unordered_map<str_t*, size_t> rawScore(longLib.size());
	//may consider parallelsm here in the future
	for (auto& str : generatedGrams)
	{
		const auto& sourceSet = ngrams[str];
		for (auto& match : sourceSet)
			rawScore[const_cast<str_t*>(match)]++;
	}
	for (auto& kp : rawScore)
		score[kp.first] = (float)kp.second / generatedGrams.size();

	/*
	float misMatchFactor = std::acosf(distanceFactor) / len / len;

	for (auto& kp : rawScore)
	{
		vector<size_t>& position = kp.second;
		size_t start = 0;
		size_t longest = 0, longestStart = 0, longestEnd = 0;
		//find longest consective sequence
		for (size_t i = 1; i < position.size(); i++)
		{
			if (i == position.size() || position[i - 1] != position[i] - 1)
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
		size_t distance = 0;
		for (size_t i = 1; i < position.size(); i++)
			distance += (size_t)abs((long long)i - (long long)longestStart) - ((long long)position[i] - (long long)position[longestStart]);

		if (distance > len * len)
			distance = len * len;
		float penalty = distanceFactor * std::cosf(misMatchFactor * distance);
		score[kp.first] += 1 - penalty;
	}
	*/
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
	if (queryStr.size() < (size_t)gramSize * 10)
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

	endIt = scoreElems.end();
	if (scoreElems.size() > limit)
		endIt = scoreElems.begin() + limit;
	result.reserve(min(limit, scoreElems.size()));
	for (auto i = scoreElems.begin(); i < endIt; i++)
		result.push_back((*i).first);
}

template<class str_t>
void StringIndex<str_t>::search(const char_t* query, char_t*** results, uint32_t* size, const float threshold, uint32_t limit)
{
	if (!indexed)
		return;

	if (limit == 0)
		limit = (numeric_limits<int32_t>::max)();

	vector<str_t> result;

	search(query, threshold, limit, result);

	//transform to C ABI using pointers
	*size = (uint32_t)result.size();
	*results = new char_t*[*size];
	for (uint32_t i = 0; i < *size; i++)
	{
		auto item = result[i];
		auto resStr = item.c_str();
		auto len = item.size();
		(*results)[i] = new char_t[len + 1]();
		memcpy((*results)[i], resStr, len);
	}
}

template<class str_t>
void StringIndex<str_t>::release(char_t*** results, size_t size) const
{
	if (*results)
	{
		for (size_t i = 0; i < size; i++)
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
// nGramSearch.cpp : Defines the exported functions for the DLL application.
//

#include "nGramSearch.h"

template<class str_t>
void StringIndex<str_t>::getGrams(size_t id)
{
	auto& str = stringLib[id];
	for (size_t i = 0; i < str.size() - gramSize + 1; i++)
	{
		auto gram = str.substr(i, gramSize);
		ngrams[gram].insert(id);
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
	for (auto& id : longLib)
		getGrams(id);
	indexed = true;
}

template<class str_t>
void StringIndex<str_t>::init(std::unordered_map<str_t, std::vector<str_t>>& tempWordMap,
	std::unordered_map<str_t, std::unordered_map<str_t, float>>& tempWordWeight)
{
	//Centralize all strings in an array so that they do not take replicated spaces
	std::unordered_set<str_t> tempStrLib;
	for (auto& kp : tempWordMap)
	{
		tempStrLib.insert(kp.first);
		for (auto& str : kp.second)
			tempStrLib.insert(str);
	}
	stringLib = std::vector<str_t>(tempStrLib.begin(), tempStrLib.end());

	//to separate the long and short libs, since different algorithms will be applied upon searches
	std::unordered_map<str_t, size_t> stringIndex(stringLib.size());
	for (size_t i = 0; i < stringLib.size(); i++)
		stringIndex[stringLib[i]] = i;
	for (auto& kp : tempWordMap)
	{
		auto& str = kp.first;
		auto pTargetIndex = stringIndex.find(str);
		if (pTargetIndex != stringIndex.end())
		{
			auto id = pTargetIndex->second;			
			if (str.size() >= (size_t)gramSize * 2)
				longLib.push_back(id);
			else
				shortLib.push_back(id);
			std::vector<size_t> keysMapped;
			keysMapped.reserve(tempWordMap[str].size());
			for (auto& key : tempWordMap[str])
			{
				auto target = stringIndex.find(key);
				if (target != stringIndex.end())
					keysMapped.push_back(target->second);
			}
			wordMap[id] = move(keysMapped);

			for (auto& pair : tempWordWeight[str])
			{
				auto target = stringIndex.find(pair.first);
				if (target != stringIndex.end())
					wordWeight[id][target->second] = pair.second;	//target.second : pointer to keyword, &str: pointer to query, pair.second: weight
			}
		}
	}

	//Each future involves an overhead of about 1-2 us, so I am limiting the number of futures to hardware concurrency
	auto nThrd = std::thread::hardware_concurrency();
	size_t sizeNeeded = (size_t)ceil((float)shortLib.size() / nThrd);
	if (sizeNeeded > sectionSize)
		sectionSize = sizeNeeded;
}

template<class str_t>
StringIndex<str_t>::StringIndex(char_t** const words, const size_t size, const uint16_t rowSize, float* const weight, const uint16_t gSize)
{
	if (gSize < 2 || size < 2)
		return;
	std::unordered_map<str_t, std::vector<str_t>> tempWordMap(size);
	std::unordered_map<str_t, std::unordered_map<str_t, float>> tempWeightMap(size);
	for (size_t i = 0; i < size; i += rowSize)
	{
		str_t strKey(words[i]);
		trim(strKey);
		if (strKey.size() == 0)
			continue;
		str_t upperKey(strKey);
		escapeBlank(upperKey);
		trim(upperKey);
		toUpper(upperKey);

		float currentWeight = 1.0f;
		if (weight)
			currentWeight = weight[i];
		tempWordMap[upperKey].push_back(strKey);
		tempWeightMap[strKey][upperKey] = currentWeight;

		for (size_t j = i + 1; j < i + rowSize; j++)
			if (words[j])
			{
				str_t strQuery(words[j]);
				escapeBlank(strQuery);
				trim(strQuery);
				toUpper(strQuery);
				if (strQuery.size() != 0)
				{
					currentWeight = 1.0f;
					if (weight)
						currentWeight = weight[j];
					tempWordMap[strQuery].push_back(strKey);
					tempWeightMap[strQuery][strKey] = currentWeight;
				}
			}
	}
	gramSize = gSize;
	init(tempWordMap, tempWeightMap);
	buildGrams();
}

template<class str_t>
StringIndex<str_t>::StringIndex(std::vector<std::vector<str_t>>& words, const int16_t gSize, std::vector<std::vector<float>>& weight)
{
	if (gSize < 2 || words.size() < 2)
		return;
	std::unordered_map<str_t, std::vector<str_t>> tempWordMap(size);
	std::unordered_map<str_t, std::unordered_map<str_t, float>> tempWeightMap(size);
	for (size_t i = 0; i < words.size(); i++)
	{
		auto& row = words[i];
		str_t strKey(row[0]);
		trim(strKey);

		if (strKey.size() == 0)
			continue;
		str_t upperKey(strKey);
		toUpper(upperKey);

		float currentWeight = 1.0f;
		if (weight)
			currentWeight = weight[i][0];

		tempWordMap[upperKey].push_back(strKey);
		tempWeightMap[strKey][upperKey] = currentWeight;

		for (size_t j = 1; j < row.size(); j++)
		{
			str_t strQuery(row[j]);
			trim(strQuery);
			if (strQuery.size() != 0)
			{
				currentWeight = 1.0f;
				if (weight)
					currentWeight = weight[i][j];
				toUpper(strQuery);
				tempWordMap[strQuery].push_back(strKey);
				tempWeightMap[strQuery][strKey] = currentWeight;
			}
		}
	}
	gramSize = gSize;
	init(tempWordMap, tempWeightMap);
	buildGrams();
}

template<class str_t>
StringIndex<str_t>::StringIndex(char_t*** const words, const size_t size, const uint16_t rowSize, float** weight, const uint16_t gSize)
{
	if (gSize < 2 || size < 2)
		return;
	std::unordered_map<str_t, std::vector<str_t>> tempWordMap(size);
	std::unordered_map<str_t, std::unordered_map<str_t, float>> tempWeightMap(size);
	for (size_t i = 0; i < size; i++)
	{
		str_t strKey(words[i][0]);
		trim(strKey);
		if (strKey.size() == 0)
			continue;
		str_t upperKey(strKey);
		escapeBlank(upperKey);
		trim(upperKey);
		toUpper(upperKey);

		float currentWeight = 1.0f;
		if (weight)
			currentWeight = weight[i][0];
		tempWordMap[upperKey].push_back(strKey);
		tempWeightMap[strKey][upperKey] = currentWeight;

		for (uint16_t j = 1; j < rowSize; j++)
			if (words[i][j])
			{
				str_t strQuery(words[i][j]);
				escapeBlank(strQuery);
				trim(strQuery);
				toUpper(strQuery);
				if (strQuery.size() != 0)
				{
					currentWeight = 1.0f;
					if (weight)
						currentWeight = weight[i][j];
					toUpper(strQuery);
					tempWordMap[strQuery].push_back(strKey);
					tempWeightMap[strQuery][strKey] = currentWeight;
				}
			}
	}
	gramSize = gSize;
	init(tempWordMap, tempWeightMap);
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
void StringIndex<str_t>::getMatchScore(const str_t& query, size_t first, std::vector<size_t>& targets, std::vector<float>& currentScore)
{
	//allocate levenstein temporary containers
	size_t maxSize = max(query.size(), (size_t)gramSize * 2 + 1);
	std::vector<size_t> row1(maxSize + 1);
	std::vector<size_t> row2(maxSize + 1);
	for (size_t i = first; i < first + sectionSize && i < shortLib.size(); i++)
	{
		auto source = shortLib[i];
		auto match = stringMatch(query, stringLib[source], row1, row2);
		currentScore[i] = (float)match / query.size();
		targets[i] = source;
	}
}

template<class str_t>
void StringIndex<str_t>::searchShort(str_t& query, std::unordered_map<size_t, float>& score)
{
	auto len = query.size();
	auto dicSize = shortLib.size();
	std::vector<size_t> targets(dicSize);
	std::vector<float> currentScore(dicSize);
	std::vector<std::future<void>> futures;
	for (size_t count = 0; count < dicSize; count += sectionSize)
		futures.emplace_back(std::async(&StringIndex::getMatchScore, this, std::cref(query), count, std::ref(targets), std::ref(currentScore)));
	for (auto& fu : futures)
		fu.get();
	for (size_t i = 0; i < dicSize; i++)
		score[targets[i]] += currentScore[i];
}


template<class str_t>
void StringIndex<str_t>::searchLong(str_t& query, std::unordered_map<size_t, float>& score)
{
	auto len = query.size();
	if (len < (size_t)gramSize)
		return;

	std::vector<str_t> generatedGrams;
	generatedGrams.reserve(len - gramSize + 1);
	getGrams(query, generatedGrams);
	if (generatedGrams.size() == 0)
		return;
	std::unordered_map<size_t, size_t> rawScore(longLib.size());
	//may consider parallelsm here in the future
	for (auto& str : generatedGrams)
	{
		const auto& sourceSet = ngrams[str];
		for (auto& match : sourceSet)
			rawScore[match]++;
	}
	for (auto& kp : rawScore)
		score[kp.first] = (float)kp.second / generatedGrams.size();
}

template<class str_t>
void StringIndex<str_t>::calcScore(std::unordered_map<size_t, float>& entryScore, std::unordered_map<size_t, float>& scoreList, const float threshold)
{
	for (auto& keyPair : scoreList)
	{
		if (keyPair.second < threshold)
			continue;
		auto& searchWord = keyPair.first;
		auto weightDicPair = wordWeight.find(searchWord);
		auto mapped = wordMap.find(searchWord);
		if (mapped != wordMap.end() && weightDicPair != wordWeight.end())
			for (auto& keyWord : mapped->second)
			{
				auto weightPair = weightDicPair->second.find(keyWord);
				if (weightPair != weightDicPair->second.end())
				{
					float itemScore = max(weightPair->second * keyPair.second, entryScore[keyWord]);
					entryScore[keyWord] = itemScore;
				}
			}
	}
}

template<class str_t>
void StringIndex<str_t>::_search(const char_t* query, const float threshold, const uint32_t limit, std::vector<size_t>& result)
{
	str_t queryStr(query);
	escapeBlank(queryStr);
	trim(queryStr);
	if (queryStr.size() == 0)
		return;
	toUpper(queryStr);
	std::unordered_map<size_t, float> scoreShort(shortLib.size());
	std::unordered_map<size_t, float> scoreLong(longLib.size());
	std::vector<std::future<void>> futures;
	//if the query is long, there is no need to search for short sequences.
	if (queryStr.size() < (size_t)gramSize * 8)
		futures.emplace_back(
			std::async(std::launch::async, &StringIndex::searchShort, this, std::ref(queryStr), ref(scoreShort))
		);
	futures.emplace_back(
		std::async(std::launch::async, &StringIndex::searchLong, this, std::ref(queryStr), ref(scoreLong))
	);
	for (auto& fu : futures)
		fu.get();

	//merge scores to entryScore
	std::unordered_map<size_t, float> entryScore(scoreShort.size() + scoreLong.size());
	calcScore(entryScore, scoreShort, threshold);
	calcScore(entryScore, scoreLong, threshold);


	std::vector<pair<size_t, float>> scoreElems(entryScore.begin(), entryScore.end());
	auto endIt = scoreElems.end();
	if (scoreElems.size() > limit)
		endIt = scoreElems.begin() + limit;
	std::partial_sort(scoreElems.begin(), endIt, scoreElems.end(), ScoreComparer<str_t>(*this));

	endIt = scoreElems.end();
	if (scoreElems.size() > limit)
		endIt = scoreElems.begin() + limit;
	result.reserve(min(limit, scoreElems.size()));
	for (auto i = scoreElems.begin(); i < endIt; i++)
		result.push_back(i->first);
}

template<class str_t>
void StringIndex<str_t>::search(const char_t* query, char_t*** results, uint32_t* size, const float threshold, uint32_t limit)
{
	if (!indexed)
		return;

	if (limit == 0)
		limit = (numeric_limits<int32_t>::max)();

	std::vector<size_t> result;

	_search(query, threshold, limit, result);

	//transform to C ABI using pointers
	*size = (uint32_t)result.size();
	*results = new char_t*[*size];
	for (uint32_t i = 0; i < *size; i++)
	{
		auto item = result[i];
		auto resStr = stringLib[item].c_str();
		auto len = stringLib[item].size();
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
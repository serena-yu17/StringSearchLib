// nGramSearch.cpp : Defines the exported functions for the DLL application.
//

#include "nGramSearch.h"

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
void StringIndex<str_t>::init(std::unordered_map<str_t, std::vector<str_t>>& tempWordMap, std::unordered_map<str_t, float>& tempWordWeight)
{
	wordMap.clear();
	shortLib.clear();
	longLib.clear();
	wordWeight.clear();
	wordMap.reserve(tempWordMap.size());
	wordWeight.reserve(tempWordWeight.size());

	std::unordered_map<str_t, size_t> shortLoc(tempWordMap.size());
	std::unordered_map<str_t, size_t> longLoc(tempWordMap.size());
	//to separate the long and short libs, since different algorithms will be applied upon searches
	for (auto& keyPair : tempWordMap)
	{
		auto& str = keyPair.first;
		if (str.size() >= (size_t)gramSize * 2)
		{
			longLib.push_back(str);
			longLoc[str] = longLib.size() - 1;
		}
		else
		{
			shortLib.push_back(str);
			shortLoc[str] = shortLib.size() - 1;
		}
	}

	//use pointers in wordMap instead of replicating the whole string to save space.
	//the pointers should be unique for each string with the above processing, so they make a good hash key
	for (auto& str : shortLib)
	{
		for (auto& key : tempWordMap[str])
		{
			str_t* pKey = nullptr;
			if (shortLoc.find(key) != shortLoc.end())
				pKey = &shortLib[shortLoc[key]];
			else if (longLoc.find(key) != longLoc.end())
				pKey = &longLib[longLoc[key]];
			if (pKey)
				wordMap[&str].push_back(pKey);
		}
		wordWeight[&str] = tempWordWeight[str];
	}
	for (auto& str : longLib)
	{
		for (auto& key : tempWordMap[str])
		{
			str_t* pKey = nullptr;
			if (shortLoc.find(key) != shortLoc.end())
				pKey = &shortLib[shortLoc[key]];
			else if (longLoc.find(key) != longLoc.end())
				pKey = &longLib[longLoc[key]];
			if (pKey)
				wordMap[&str].push_back(pKey);
		}
		wordWeight[&str] = tempWordWeight[str];
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
	std::unordered_map<str_t, float> tempWeightMap(size);
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
		tempWeightMap[upperKey] = currentWeight;

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
					tempWeightMap[strQuery] = currentWeight;
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
	std::unordered_map<str_t, float> tempWeightMap(size);
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
		tempWeightMap[upperKey] = currentWeight;

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
				tempWeightMap[strQuery] = currentWeight;
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
	std::unordered_map<str_t, float> tempWeightMap(size);
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
		tempWeightMap[upperKey] = currentWeight;

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
					tempWeightMap[strQuery] = currentWeight;
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
void StringIndex<str_t>::getMatchScore(const str_t& query, size_t first, std::vector<str_t*>& targets, std::vector<float>& currentScore)
{
	//allocate levenstein temporary containers
	size_t maxSize = max(query.size(), (size_t)gramSize * 2 + 1);
	std::vector<size_t> row1(maxSize + 1);
	std::vector<size_t> row2(maxSize + 1);
	for (size_t i = first; i < first + sectionSize && i < shortLib.size(); i++)
	{
		str_t& source = shortLib[i];
		auto match = stringMatch(query, source, row1, row2);
		currentScore[i] = (float)match / query.size();
		targets[i] = &source;
	}
}

template<class str_t>
void StringIndex<str_t>::searchShort(str_t& query, std::unordered_map<str_t*, float>& score)
{
	auto len = query.size();
	auto dicSize = shortLib.size();
	std::vector<str_t*> targets(dicSize);
	std::vector<float> currentScore(dicSize);
	std::vector<std::future<void>> futures;
	for (size_t count = 0; count < dicSize; count += sectionSize)
		futures.emplace_back(std::async(&StringIndex::getMatchScore, this, std::cref(query), count, std::ref(targets), std::ref(currentScore)));
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
void StringIndex<str_t>::searchLong(str_t& query, std::unordered_map<str_t*, float>& score)
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
void StringIndex<str_t>::calcScore(std::unordered_map<str_t*, float>& entryScore, std::unordered_map<str_t*, float>& scoreList, const float threshold)
{
	for (auto& keyPair : scoreList)
	{
		if (keyPair.second < threshold)
			continue;
		str_t* term = keyPair.first;
		auto weight = wordWeight.find(term);
		auto mapped = wordMap.find(term);
		if (mapped != wordMap.end() && weight != wordWeight.end())
			for (auto& item : mapped->second)
			{
				float itemScore = max(weight->second * keyPair.second, entryScore[item]);
				entryScore[item] = itemScore;
			}
	}
}

template<class str_t>
void StringIndex<str_t>::_search(const char_t* query, const float threshold, const uint32_t limit, std::vector<str_t*>& result)
{
	str_t queryStr(query);
	escapeBlank(queryStr);
	trim(queryStr);
	if (queryStr.size() == 0)
		return;
	toUpper(queryStr);
	std::unordered_map<str_t*, float> scoreShort(shortLib.size());
	std::unordered_map<str_t*, float> scoreLong(longLib.size());
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
	std::unordered_map<str_t*, float> entryScore(scoreShort.size() + scoreLong.size());	
	calcScore(entryScore, scoreShort, threshold);
	calcScore(entryScore, scoreLong, threshold);
	

	std::vector<pair<str_t*, float>> scoreElems(entryScore.begin(), entryScore.end());	
	auto endIt = scoreElems.end();
	if (scoreElems.size() > limit)
		endIt = scoreElems.begin() + limit;
	std::partial_sort(scoreElems.begin(), endIt, scoreElems.end(), compareScores<str_t>);

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

	std::vector<str_t*> result;

	_search(query, threshold, limit, result);

	//transform to C ABI using pointers
	*size = (uint32_t)result.size();
	*results = new char_t*[*size];
	for (uint32_t i = 0; i < *size; i++)
	{
		auto item = result[i];
		auto resStr = item->c_str();
		auto len = item->size();
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
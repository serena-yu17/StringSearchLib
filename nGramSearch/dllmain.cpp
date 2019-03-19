// dllmain.cpp : Defines the entry point for the DLL application.
#include "nGramSearch.h"
#include "nGramSearch.hpp"

#if defined(_MSC_VER)
	//  MSVC
#define DLLEXP extern "C" __declspec(dllexport)
#elif defined(__GNUC__)
	//  GCC
#define DLLEXP extern "C"  __attribute__((visibility("default")))
#elif defined(__clang__)
	//clang
#define DLLEXP extern "C"  __attribute__((visibility("default")))
#else
#define DLLEXP extern "C"
#pragma warning Unknown dynamic link import/export semantics
#endif

using namespace std;
using namespace StringSearch;

std::shared_mutex mainLock;
//key entries for indexed StringIndex class instances
unordered_map<uint32_t, unique_ptr<StringIndex>> indexed;


/*!
Index the library based on a string array of key, and another array of additional text, e.g. description.
@param words Words to be searched for. For each row, the first word is used as the master key, in which the row size is \p rowSize.
All rows are flattened into a 1D-array, and can be extracted based on \p rowSize.
In a search, all queries of the words in a row will return the master key.
@param size size of the \p words
@param rowSize size of each text rows of \p words.
@param weight A list of weight values for each key. It should be at least as long as the number of rows, i.e. \p size / \p rowSize.
@returns handle to the library
*/
DLLEXP uint32_t indexN(char** const words, const uint64_t size, const uint16_t rowSize, float* const weight)
{
	unique_lock<shared_mutex> updLock(mainLock);
	//0 is reserved to represent an empty handle
	uint32_t handle = 1;
	const uint32_t maxVal = (numeric_limits<uint32_t>::max)();
	while (indexed.find(handle) != indexed.end() && handle < maxVal)
		handle++;
	if (handle == maxVal)
		return 0;
	indexed.emplace(handle, make_unique<StringIndex>(words, (size_t)size, rowSize, weight));
	return handle;
}

/*!
Search the query in the indexed library identified by the guid.
@param handle A unique id for the indexed library
@param query The query string
@param results The pointer to a string array for output. The memory will be allocated by new.
Must call \p release to clean up after use.
@param size Output the length of the \p results array.
@param threshold Lowest acceptable matching %, as a value between 0 and 1
@param limit Maximum results generated
*/
DLLEXP uint32_t search(uint32_t handle, const char* query, char*** results, float threshold, uint32_t limit)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto pkeyPair = indexed.find(handle);
	if (pkeyPair != indexed.end() && pkeyPair->second)
	{
		return pkeyPair->second->search(query, results, threshold, limit);
	}
	return 0;
}

/*!
Search the query in the indexed library identified by the guid.
@param handle A unique id for the indexed library
@param query The query string
@param results The pointer to a string array for output. The memory will be allocated by new.
Must call \p release to clean up after use.
@param size Output the length of the \p results array.
@param threshold Lowest acceptable matching %, as a value between 0 and 1
@param limit Maximum results generated
*/
DLLEXP uint32_t score(uint32_t handle, const char* query, char*** results, float** scores, float threshold, uint32_t limit)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto pkeyPair = indexed.find(handle);
	if (pkeyPair != indexed.end() && pkeyPair->second)
	{
		return pkeyPair->second->score(query, results, scores, threshold, limit);
	}
}

/*!
To release the memory allocated for the result in the \p search function
@param handle A unique id for the indexed library
@param results The result returned by the <search> function.
@param size Length of \p result
*/
DLLEXP void release(uint32_t handle, char** results, float* scores)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto keyPair = indexed.find(handle);
	if (keyPair != indexed.end() && keyPair->second)
		keyPair->second->release(results, scores);
}

/*!
To dispose a library indexed. If the library does not exist, \p dispose will ignore it.
@param handle A unique id for the indexed library
*/
DLLEXP void dispose(uint32_t handle)
{
	unique_lock<shared_mutex> updLock(mainLock);
	indexed.erase(handle);
}

/*!
To obtain the current word map size
@param handle A unique id for the indexed library
*/
DLLEXP uint64_t getSize(uint32_t handle)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto keyPair = indexed.find(handle);
	if (keyPair != indexed.end() && keyPair->second)
		return keyPair->second->size();
	return 0;
}

/*!
To obtain the current n-gram library size.
@param guid A unique id for the indexed library
*/
DLLEXP uint64_t getLibSize(uint32_t handle)
{
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto keyPair = indexed.find(handle);
	if (keyPair != indexed.end() && keyPair->second)
		return keyPair->second->libSize();
	return 0;
}

DLLEXP void setValidChar(uint32_t handle, char* const characters, int n)
{
	std::unordered_set<char> newValidChar(n);
	for (int i = 0; i < n; i++)
		newValidChar.insert(characters[i]);
	shared_lock<shared_mutex> sharedLock(mainLock);
	auto keyPair = indexed.find(handle);
	if (keyPair != indexed.end() && keyPair->second)
		keyPair->second->setValidChar(newValidChar);
}


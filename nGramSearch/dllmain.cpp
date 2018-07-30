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

shared_timed_mutex mainLock;
//key entries for indexed StringIndex class instances
unordered_map<string, unique_ptr<StringIndex<string>>> indexed;
unordered_map<string, unique_ptr<StringIndex<wstring>>> indexedW;

/*!
Index the library based on a 2D array.
@param words For each row, the first string is the key to be mapped to, and the second string is the description mapped to the key
@param guid A unique id for the indexed library
@param size size of the \p words
@param gSize size of grams to be created. Default 3
@param weight A list of the relative weight of each key. Default 1 for all
*/
DLLEXP void index2D(char* const guid, char*** const words, const uint64_t size, const uint16_t rowSize, float** const weight, const uint16_t gSize)
{
	std::unique_lock<shared_timed_mutex> updLock(mainLock);
	indexed.emplace(string(guid), make_unique<StringIndex<string>>(words, (size_t)size, rowSize, weight, gSize));
}

/*!
Index the library based on a 2D array. Wide string version
@param words For each row, the first string is the key to be mapped to, and the second string is the description mapped to the key
@param guid A unique id for the indexed library
@param size size of the \p words
@param gSize size of grams to be created. Default 3
@param weight A list of the relative weight of each key. Default 1 for all
*/
DLLEXP void index2DW(char* const guid, wchar_t*** const words, const uint64_t size, const uint16_t rowSize, float** const weight, const uint16_t gSize)
{
	unique_lock<shared_timed_mutex> updLock(mainLock);
	indexedW.emplace(string(guid), make_unique<StringIndex<wstring>>(words, (size_t)size, rowSize, weight, gSize));
}

/*!
Index the library based on a string array of key, and another array of additional text, e.g. description.
@param guid A unique id for the indexed library
@param words Words to be searched for. For each row, the first word is used as the master key, in which the row size is \p rowSize.
All rows are flattened into a 1D-array, and can be extracted based on \p rowSize.
In a search, all queries of the words in a row will return the master key.
@param size size of the \p words
@param rowSize size of each text rows of \p words.
@param weight A list of weight values for each key. It should be at least as long as the number of rows, i.e. \p size / \p rowSize.
@param gSize size of grams to be created. Default 3.
*/
DLLEXP void indexN(char* const guid, char** const words, const uint64_t size, const uint16_t rowSize, float* const weight, const uint16_t gSize)
{
	unique_lock<shared_timed_mutex> updLock(mainLock);
	indexed.emplace(string(guid), make_unique<StringIndex<string>>(words, (size_t)size, rowSize, weight, gSize));
}

/*!
Index the library based on a string array of key, and another array of additional text, e.g. description.
Wide string version.
@param guid A unique id for the indexed library
@param words Words to be searched for. For each row, the first word is used as the master key, in which the row size is \p rowSize.
All rows are flattened into a 1D-array, and can be extracted based on \p rowSize.
In a search, all queries of the words in a row will return the master key.
@param size size of the \p words
@param rowSize size of each text rows of \p words.
@param weight A list of weight values for each key. It should be at least as long as the number of rows, i.e. \p size / \p rowSize.
@param gSize size of grams to be created. Default 3.
*/
DLLEXP void indexW(char* const guid, wchar_t** const words, const uint64_t size, const uint16_t rowSize, float* const weight, const uint16_t gSize)
{
	unique_lock<shared_timed_mutex> updLock(mainLock);
	indexedW.emplace(string(guid), make_unique<StringIndex<wstring>>(words, (size_t)size, rowSize, weight, gSize));
}

/*!
Search the query in the indexed library identified by the guid.
@param guid A unique id for the indexed library
@param query The query string
@param results The pointer to a string array for output. The memory will be allocated by new.
Must call \p release to clean up after use.
@param size Output the length of the \p results array.
@param threshold Lowest acceptable matching %, as a value between 0 and 1
@param limit Maximum results generated
*/
DLLEXP uint32_t search(char* const guid, const char* query, char*** results, uint32_t* size, const float threshold, uint32_t limit)
{
	shared_lock<shared_timed_mutex> sharedLock(mainLock);
	auto pkeyPair = indexed.find(string(guid));
	if (pkeyPair != indexed.end())
	{
		auto& instance = pkeyPair->second;
		//sharedLock.unlock();
		return instance->search(query, results, size, threshold, limit);
	}
	return 0;
}

/*!
Search the query in the indexed library identified by the guid.
Wide string version
@param guid A unique id for the indexed library
@param query The query string
@param results The pointer to a string array for output. The memory will be allocated by new.
Must call \p releaseW to clean up after use.
@param size Output the length of the \p results array.
@param threshold Lowest acceptable matching %, as a value between 0 and 1
@param limit Maximum results generated
*/
DLLEXP uint32_t searchW(char* const guid, const wchar_t* query, wchar_t*** results, uint32_t* size, const float threshold, uint32_t limit)
{
	shared_lock<shared_timed_mutex> sharedLock(mainLock);
	auto pkeyPair = indexedW.find(string(guid));
	if (pkeyPair != indexedW.end())
	{
		auto& instance = pkeyPair->second;
		//sharedLock.unlock();
		return instance->search(query, results, size, threshold, limit);
	}
	return 0;
}

/*!
To release the memory allocated for the result in the \p search function
@param guid A unique id for the indexed library
@param results The result returned by the <search> function.
@param size Length of \p result
*/
DLLEXP void release(char* const guid, char*** results, uint64_t size)
{
	shared_lock<shared_timed_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		(instance->second)->release(results, (size_t)size);
}

/*!
To release the memory allocated for the result in the <search> function.
Wide string version.
@param guid A unique id for the indexed library
@param results The result returned by the <search> function.
@param size Length of \p result
*/
DLLEXP void releaseW(char* const guid, wchar_t*** results, uint64_t size)
{
	shared_lock<shared_timed_mutex> sharedLock(mainLock);
	auto instance = indexedW.find(string(guid));
	if (instance != indexedW.end())
		(instance->second)->release(results, (size_t)size);
}

/*!
To dispose a library indexed. If the library does not exist, \p dispose will ignore it.
@param guid A unique id for the indexed library
*/
DLLEXP void dispose(char* const guid)
{
	unique_lock<shared_timed_mutex> updLock(mainLock);
	indexed.erase(string(guid));
}

/*!
To dispose a library indexed. If the library does not exist, \p dispose will ignore it.
Wide string version.
@param guid A unique id for the indexed library
*/
DLLEXP void disposeW(char* const guid)
{
	unique_lock<shared_timed_mutex> updLock(mainLock);
	indexedW.erase(string(guid));
}

/*!
To obtain the current word map size
@param guid A unique id for the indexed library
*/
DLLEXP uint64_t getSize(char* const guid)
{
	shared_lock<shared_timed_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		return instance->second->size();
	return 0;
}

/*!
To obtain the current word map size
Wide string version.
@param guid A unique id for the indexed library
*/
DLLEXP uint64_t getSizeW(char* const guid)
{
	shared_lock<shared_timed_mutex> sharedLock(mainLock);
	auto instance = indexedW.find(string(guid));
	if (instance != indexedW.end())
		return instance->second->size();
	return 0;
}

/*!
To obtain the current n-gram library size.
@param guid A unique id for the indexed library
*/
DLLEXP uint64_t getLibSize(char* const guid)
{
	shared_lock<shared_timed_mutex> sharedLock(mainLock);
	auto instance = indexed.find(string(guid));
	if (instance != indexed.end())
		return instance->second->libSize();
	return 0;
}

/*!
To obtain the current n-gram library size.
wide string version.
@param guid A unique id for the indexed library
*/
DLLEXP uint64_t getLibSizeW(char* const guid)
{
	shared_lock<shared_timed_mutex> sharedLock(mainLock);
	auto instance = indexedW.find(string(guid));
	if (instance != indexedW.end())
		return instance->second->libSize();
	return 0;
}


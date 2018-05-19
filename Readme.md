# This project is a C++ fuzzy string search library based on the n-gram algorithm.



## DLL Interface:



#### Index the library based on a 2D array.

`void index2D(char* const guid, char*** const key, const uint64_t size, const uint16_t rowSize, float** const weight = NULL, const uint16_t gSize = 3);`

`words` For each row, the first string is the key to be mapped to, and the second string is the description mapped to the key

`guid` A unique id for the indexed library

`size` size of the `words`

`gSize` size of grams to be created. Default 3

`weight` A list of the relative weight of each key. Default 1 for all



#### Index the library based on a string array of key, and another array of additional text, e.g. description.

`void index2DW(char* const guid, wchar_t*** const key, const uint64_t size, const uint16_t rowSize, float** const weight = NULL, const uint16_t gSize = 3)`

`guid` A unique id for the indexed library

`words` Words to be searched for. For each row, the first word is used as the master key, in which the row size is `rowSize`.

All rows are flattened into a 1D-array, and can be extracted based on `rowSize`. 

In a search, all queries of the words in a row will return the master key.

`size` size of the `words`

`rowSize` size of each text rows of `words`.

`weight` A list of weight values for each key. It should be at least as long as the number of rows, i.e. `size` / `rowSize`.

`gSize` size of grams to be created. Default 3.



#### Index the library based on a string array of key, and another array of additional text, e.g. description.

`void index(char* const guid, char** const key, const uint64_t size, const uint16_t rowSize, float* const weight = NULL, const uint16_t gSize = 3)`

`guid` A unique id for the indexed library

`words` Words to be searched for. For each row, the first word is used as the master key, in which the row size is `rowSize`.

All rows are flattened into a 1D-array, and can be extracted based on `rowSize`. 

In a search, all queries of the words in a row will return the master key.

`size` size of the `words`

`rowSize` size of each text rows of `words`.

`weight` A list of weight values for each key. It should be at least as long as the number of rows, i.e. `size` / `rowSize`.

`gSize` size of grams to be created. Default 3.



#### Index the library based on a string array of key, and another array of additional text, e.g. description.

`void indexW(char* const guid, wchar_t** const key, const uint64_t size, const uint16_t rowSize, float* const weight = NULL, const uint16_t gSize = 3)`

Wide string version.

`guid` A unique id for the indexed library

`words` Words to be searched for. For each row, the first word is used as the master key, in which the row size is `rowSize`.

All rows are flattened into a 1D-array, and can be extracted based on `rowSize`.

In a search, all queries of the words in a row will return the master key.

`size` size of the `words`

`rowSize size of each text rows of `words`.

`weight A list of weight values for each key. It should be at least as long as the number of rows, i.e. `size` / `rowSize`.

`gSize` size of grams to be created. Default 3.



#### Search the query in the indexed library identified by the guid.

`void search(char* const guid, const char* query, char*** results, uint32_t* size, const float threshold = 0, uint32_t limit = 100)`

`guid` A unique id for the indexed library

`query` The query string

`results The pointer to a string array for output. The memory will be allocated by new.

Must call `release` to clean up after use.

`size` Output the length of the `results` array.

`threshold` Lowest acceptable matching %, as a value between 0 and 1

`limit` Maximum results generated



#### Search the query in the indexed library identified by the guid.

`void searchW(char* const guid, const wchar_t* query, wchar_t*** results, uint32_t* size, const float threshold = 0, uint32_t limit = 100)`

Wide string version

`guid` A unique id for the indexed library

`query` The query string

`results` The pointer to a string array for output. The memory will be allocated by new. 

Must call `releaseW` to clean up after use.

`size` Output the length of the `results` array.

`threshold` Lowest acceptable matching %, as a value between 0 and 1

`limit` Maximum results generated



#### To release the memory allocated for the result in the `search` function

`void release(char* const guid, char*** results, uint64_t size)`

`guid` A unique id for the indexed library

`results` The result returned by the <search> function.

`size` Length of `result`



#### To release the memory allocated for the result in the <search> function.

Wide string version.

`void releaseW(char* const guid, wchar_t*** results, uint64_t size)`

`guid` A unique id for the indexed library

`results` The result returned by the <search> function.

`size` Length of `result`



#### To dispose a library indexed. If the library does not exist, `dispose will ignore it.

`void dispose(char* const guid)`

`guid` A unique id for the indexed library



#### To dispose a library indexed. If the library does not exist, `dispose will ignore it.

`void disposeW(char* const guid)`

Wide string version.

`guid` A unique id for the indexed library



#### To obtain the current word map size

`uint64_t getSize(char* const guid)`

`guid` A unique id for the indexed library



#### To obtain the current word map size

`uint64_t getSizeW(char* const guid)`

Wide string version.

`guid` A unique id for the indexed library



#### To obtain the current n-gram library size.

`uint64_t getLibSize(char* const guid)`

`guid` A unique id for the indexed library



#### To obtain the current n-gram library size.

`uint64_t getLibSizeW(char* const guid)`

wide string version.

`guid` A unique id for the indexed library

---

## Usage:

```
if (!unicode)
    index(guid, words, size, rowSize, weight, gramSize);
else
    indexW(guid, words, size, rowSize, weight, gramSize);
```

```
if (!unicode)
    search(guid, query, pResult, pCount, threshold, limit);
else
    searchW(guid, query, pResult, pCount, threshold, limit);
```
This project is a C++ fuzzy string search library based on the n-gram algorithm.

DLL Interface:
void index2D(char* const guid, char*** const key, const size_t size, const uint16_t gSize = 3, float* weight = NULL)

Takes a 2D string array [key], for each row, the second string will be mapped to the first one. 
[size] number of rows in the array
[guid] a global key to identify the data indexed
[gSize] gram size, default 3
[weight] weight correction for each row 

...to be continued
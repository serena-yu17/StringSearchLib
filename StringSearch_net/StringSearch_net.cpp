#include "stdafx.h"

#include "StringSearch_net.h"
#include "nGramSearch.h"


public ref class StringSearch : IDisposable
{
private:
	bool _indexed = false;

public:
	StringSearch(array<String^>^ keys, array<String^>^ additional)
	{

	}

	bool indexed()
	{
		return _indexed;
	}

	void overide Dispose()
	{

	}
};



#include "pch.h"
#include "nGramSearch.h"
#include "dllmain.cpp"

int handle = 0;
void SetUp() {
	char** words = new char*[7]{
		"LWMS", "LWM", "LWMA", "LWYY", "L", "I", "GHRSDGSDGS Egdsrtg g"
	};
	handle = indexN(words, 7, 7, NULL);
}

TEST(StringTest, test_for_search) {
	EXPECT_EQ(7, getSize(handle));
	EXPECT_EQ(16, getLibSize(handle));
	char*** result = nullptr;
	uint32_t* size = 0;
	auto perfCount = search(handle, "LWMS", result, size, 0.5f, (numeric_limits<int>::max)());
	EXPECT_EQ(4, perfCount);
}
#include "../trie.hpp"
#include "gtest/gtest.h"

static std::vector<std::string> words = {
	"jail","afterthought","nippy","gifted","tiger","snore","part","alike",
	"tangy","dry","hesitant","building","interrupt","diligent","move","spare",
	"soggy","petite","observe","ready","stitch","brick","print","skin","pinch",
	"history","hands","treat","prefer","tent","shallow","stain","quick","like",
	"brawny","apologise","daily","hard","explode","long-term","dusty","teeth",
	"hunt","comparison","rod","one","dance","shelter","fancy","fine", "burrito"
};

static std::vector<std::wstring> wwords = {
	L"jail", L"afterthought", L"nippy", L"gifted", L"tiger", L"snore", L"part",
	L"alike", L"tangy", L"dry", L"hesitant", L"building", L"interrupt",
	L"diligent", L"move", L"spare", L"soggy", L"petite", L"observe", L"ready",
	L"stitch", L"brick", L"print", L"skin", L"pinch", L"history", L"hands",
	L"treat", L"prefer", L"tent", L"shallow", L"stain", L"quick", L"like", 
	L"brawny", L"apologise", L"daily", L"hard", L"explode", L"long-term", 
	L"dusty", L"teeth", L"hunt", L"comparison", L"rod", L"one", L"dance", 
	L"shelter", L"fancy", L"fine"
};

TEST(trie, add_remove) {
	trie<char> t;

	for (auto &s : words) {
		t.add(s);
	}
	ASSERT_EQ(t.size(), words.size());

	for (auto &s : words) {
		t.remove(s);
	}
	ASSERT_EQ(t.size(), 0);
}

TEST(trie, char_suggestions) {
	trie<char> t;
	for (auto &s : words) {
		t.add(s);
	}
	std::vector<std::string> expected_a{
		"afterthought",
		"alike",
		"apologise"
	};
	ASSERT_EQ(t.suggestions("a"), expected_a);

	std::vector<std::string> expected_j = {
		"jail"
	};

	ASSERT_EQ(t.suggestions("j"), expected_j);
}

TEST(trie, wchar_t_suggestions) {
	trie<wchar_t> t;
	for (auto &s : wwords) {
		t.add(s);
	}
	std::vector<std::wstring> expected_a{
		L"afterthought",
		L"alike",
		L"apologise"
	};
	ASSERT_EQ(t.suggestions(L"a"), expected_a);

	std::vector<std::wstring> expected_j = {
		L"jail"
	};

	ASSERT_EQ(t.suggestions(L"j"), expected_j);
}


#ifdef EXPERIMENTAL_CORO
TEST(trie, coro) {
	trie<char> t;

	for (auto &s : words) {
		t.add(s);
	}
	std::vector<std::string> expected = {
		"afterthought",
		"alike",
		"apologise"
	};

	auto gen = t.lazy_suggestions("a");
	auto it = gen.begin();

	std::vector<std::string> actual;
	actual.push_back(*it);
	actual.push_back(*(++it));
	actual.push_back(*(++it));

	ASSERT_EQ(actual, expected);
}
#endif

int main(int argc, char *argv[]) {
	testing::InitGoogleTest(&argc, argv);
	int res = RUN_ALL_TESTS();

	system("PAUSE");
	return res;
}
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

TEST(utils, node_to_char_string) {
	trie<char> t;
	t.add("playing");
	std::string expected = "play";

	const auto *n = t.find_prefix("play");

	ASSERT_EQ(utils::node_to_string(n), expected);
}

TEST(utils, node_to_wchar_string) {
	trie<wchar_t> t;
	t.add(L"intermediate");
	std::wstring expected = L"inter";

	const auto *n = t.find_prefix(L"inter");

	ASSERT_EQ(utils::node_to_string(n), expected);
}

TEST(node, paths_to) {
	trie<char> t;

	t.add("italy");
	const trie<char>::node *n = t.find_prefix("i");

	trie<char>::node::path_list expected = {
		t.find_prefix("ita")
	};
	ASSERT_EQ(n->paths_to('a', 0), expected);

	expected = {};
	ASSERT_EQ(n->paths_to('a', 4), expected);

	t.add("iran");
	t.add("icarus");

	expected = {
		t.find_prefix("ica"),
		t.find_prefix("ira"),
		t.find_prefix("ita")
	};
	ASSERT_EQ(n->paths_to('a'), expected);

	expected.erase(begin(expected) + 1); // rm iran
	ASSERT_EQ(n->paths_to('a', 2), expected);

	expected.erase(begin(expected) + 1); // rm italy
	ASSERT_EQ(n->paths_to('a', 3), expected);


}
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
	ASSERT_EQ(t.complete_suggestions("a"), expected_a);

	std::vector<std::string> expected_j = {
		"jail"
	};

	ASSERT_EQ(t.complete_suggestions("j"), expected_j);
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
	ASSERT_EQ(t.complete_suggestions(L"a"), expected_a);

	std::vector<std::wstring> expected_j = {
		L"jail"
	};

	ASSERT_EQ(t.complete_suggestions(L"j"), expected_j);
}

TEST(trie, closest_match) {
	trie<char> t;

	std::vector<std::string> words = {
		"after",
		"amo",
		"ami",
		"ama",
		"exact"
	};

	for (auto &s : words) {
		t.add(s);
	}

	// Test word length
	ASSERT_EQ(t.closest_matches("aftert").size(), 0);
	ASSERT_EQ(t.closest_matches("ame").size(), 3);

	// Test last character typo
	ASSERT_EQ(t.closest_matches("ame").size(), 3);

	// Test second-to-last character typo
	ASSERT_EQ(t.closest_matches("avo").size(), 1);


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
#include <benchmark\benchmark.h>
#include "../trie.hpp"
#include "../trie_vec.hpp"
#include <iostream>
#include <random>
#include <algorithm>
#include <new>
#include "comparison.h"

std::random_device rd;
std::uniform_int_distribution<int> dist('A', '}');

template <class ValueT, size_t MaxLen = 255U, class Traits = std::char_traits<ValueT>>
using vec_trie = trie<ValueT, MaxLen, Traits, impl_::default_vector_storage, impl_::default_vector_accessor>;


template <class ValueT,size_t MaxLen = 255U, class Traits = std::char_traits<ValueT>>
using set_trie = trie<ValueT, MaxLen, Traits, impl_::default_set_storage, impl_::default_set_storage_accessor>;

#ifdef _MSC_VER 
#pragma comment(lib, "Shlwapi.lib")
#endif

std::vector<std::string> generate_random_words(int count, int len) {
	std::string s(len, 'a');
	std::vector<std::string> res;
	res.reserve(count);

	for (int j = 0; j < count; ++j) {
		std::mt19937 rnd(rd());
		std::generate(s.begin(), s.end(), [&] { return dist(rnd); });
		res.push_back(s);
	}
	return res;
}

static void BM_SetTrieAdd255(benchmark::State& state) {
	
	while (state.KeepRunning()) {
		state.PauseTiming();
		set_trie<char, 255U> t;
		auto vec = generate_random_words(state.range(0), state.range(1));
		state.ResumeTiming();
		for (const auto & s : vec)
			t.add(s);
	}

	state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_VecTrieAdd255(benchmark::State& state) {

	while (state.KeepRunning()) {
		state.PauseTiming();
		vec_trie<char, 255U> t;
		auto vec = generate_random_words(state.range(0), state.range(1));
		state.ResumeTiming();
		for (const auto & s : vec)
			t.add(s);
	}

	state.SetItemsProcessed(state.iterations() * state.range(0));
}


static void BM_SetTrieAdd512(benchmark::State& state) {
	while (state.KeepRunning()) {
		state.PauseTiming();
		set_trie<char, 512U> t;
		auto vec = generate_random_words(state.range(0), state.range(1));
		state.ResumeTiming();
		for (const auto & s : vec)
			t.add(s);
	}

	state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_TrieAddComp(benchmark::State& state) {
	while (state.KeepRunning()) {
		state.PauseTiming();
		Trie t;
		auto words = generate_random_words(state.range(0), state.range(1));
		state.ResumeTiming();
		t.build_trie(words.data(), words.size());
	}

	state.SetItemsProcessed(state.iterations() * state.range(0));
}

#ifdef BENCH_ADD
BENCHMARK(BM_SetTrieAdd255)->Ranges({ { 64, 4096 }, { 8, 64 } })->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_VecTrieAdd255)->Ranges({ { 64, 4096 },{ 8, 255 } })->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TrieAddComp)->Ranges({ { 1, 4096 },{ 1, 64 } })->Unit(benchmark::kMicrosecond);
#endif

// Define another benchmark
static void BM_SetTrieFind(benchmark::State& state) {
	while (state.KeepRunning()) {
		state.PauseTiming();
		set_trie<char, 255U> t;
		auto words = generate_random_words(state.range(0), state.range(1));
		for (const auto &word : words) {
			t.add(word);
		}
		std::vector<std::string> s;
		std::sample(words.begin(), words.end(), std::back_inserter(s), state.range(2), std::mt19937{ std::random_device{}() });
		state.ResumeTiming();

		for (const auto &str : s)
			t.find_prefix(str);
	}
}

static void BM_VecTrieFind(benchmark::State& state) {
	while (state.KeepRunning()) {
		state.PauseTiming();
		vec_trie<char, 255U> t;
		auto words = generate_random_words(state.range(0), state.range(1));
		for (const auto &word : words) {
			t.add(word);
		}
		std::vector<std::string> s;
		std::sample(words.begin(), words.end(), std::back_inserter(s), state.range(2), std::mt19937{ std::random_device{}() });
		state.ResumeTiming();

		for (const auto &str : s)
			t.find_prefix(str);
	}
}

static void BM_TrieFindComp(benchmark::State& state) {

	while (state.KeepRunning()) {
		state.PauseTiming();
		Trie t;
		auto words = generate_random_words(state.range(0), state.range(1));
		t.build_trie(words.data(), words.size());
		std::vector<std::string> s;
		std::sample(words.begin(), words.end(), std::back_inserter(s), state.range(2), std::mt19937{ std::random_device{}() });
		bool res;
		state.ResumeTiming();
		for (const auto &str : s)
			(t.search(str, res));
	}
}
std::vector<std::pair<int, int>> ranges = { { 512, 4096 },{ 16, 255 } ,{1, 64} };
BENCHMARK(BM_SetTrieFind)->Ranges(ranges)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_VecTrieFind)->Ranges(ranges)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TrieFindComp)->Ranges(ranges)->Unit(benchmark::kMicrosecond);

int main(int argc, char** argv) {
	//std::cout << sizeof(trie<char, 255>::node) << std::endl << sizeof(trie<char, 512>::node) << std::endl;
	::benchmark::Initialize(&argc, argv);  
	if (::benchmark::ReportUnrecognizedArguments(argc, argv)) 
		return 1; 
	::benchmark::RunSpecifiedBenchmarks();
	system("PAUSE");
}
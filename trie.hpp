#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <set>

#define EXPERIMENTAL_CORO

#ifdef EXPERIMENTAL_CORO
#include <experimental\coroutine>
#include <experimental\generator>
#endif

namespace impl_
{
template <class ValueT>
class node_t 
{
	struct node_less {
		bool operator () (const node_t &lhs, const node_t &rhs) const{
			return lhs.value() < rhs.value();
		}
		bool operator () (const node_t &lhs, const ValueT &rhs) const{
			return lhs.value() < rhs;
		}
		bool operator () (const ValueT &lhs, const node_t &rhs) const{
			return lhs < rhs.value();
		}
		struct is_transparent {};
	};

public:
	node_t(node_t *parent, ValueT ch, bool marked = false) :
		parent_(parent), value_(ch)
	{
		marked_ = marked;
		leaf_ = true;
	}

	node_t *emplace_child(ValueT c, bool marked = false) {
		leaf_ = false;

		return const_cast<node_t*>(&*(children_.emplace(this, c, marked).first));
	}

	node_t *get_or_emplace(ValueT c) {
		node_t *res = get_child(c);
		if (!res) {
			res = emplace_child(c);
		}

		return res;
	}

	void remove_child(ValueT v) {
		children_.erase(children_.find(v));
	}

	void mark() {
		marked_ = 1;
	}

	void unmark() {
		marked_ = 0;
	}

	bool marked() const {
		return marked_;
	}

	bool leaf() const {
		return leaf_;
	}

	ValueT value() const {
		return value_;
	}

	node_t *get_child(ValueT ch) {
		auto it = children_.find(ch);

		if (it == end(children_)) return nullptr;

		return const_cast<node_t *>(&*it); // I promise I won't change the contained value
	}
	const node_t *get_child(ValueT ch) const {
		return const_cast<node_t *>(this)->get_child(ch);
	}
	const auto &get_children() const {
		return children_;
	}

	const node_t *parent() const {
		return parent_;
	}

	node_t *parent() {
		return parent_;
	}

private:
	std::set<node_t, node_less> children_;
	node_t *parent_;
	ValueT value_;

	unsigned leaf_ : 1;
	unsigned marked_ : 1;
	unsigned collapsed_ : 1;
};
}// namespace impl_

template <class CharT = char>
class trie
{
public:
	using node = impl_::node_t<CharT>;
	using string = std::basic_string<CharT>;

	void add(const string &s) {
		node *current = &root_;
		for (size_t j = 0, len = s.length(); j < len; ++j) {
			current = current->get_or_emplace(s[j]);
		}
		if (!current->marked()) {
			++size_;
			current->mark();
		}
	}

	node *find_incomplete(const string &s) {
		node *current = &root_;
		for (size_t j = 0, len = s.length(); j < len; ++j) {
			node *it = current->get_child(s[j]);
			if (!it) return nullptr;
			current = it;
		}

		return current;
	}

	const node *find_incomplete(const string &s) const {
		return const_cast<trie *>(this)->find_incomplete(s);
	}

	std::vector<string> suggestions(const string &s) {
		const node *it = find_incomplete(s);
		if (!it) return{};

		auto sug = suggestions_impl(*it, s);
		if (it->marked()) {
			sug.push_back(s);
			std::iter_swap(begin(sug), end(sug) - 1);
		}
		return sug;
	}

#ifdef EXPERIMENTAL_CORO
	std::experimental::generator<string> lazy_suggestions(string s) const {
		const node *it = find_incomplete(s);
		if (!it) co_return;

		if (it->marked()) {
			co_yield s;
		}
		
		for (const auto &str : lazy_suggestions_impl(*it, s)) {
			co_yield str;
		}
	}
#endif
	
	void remove(const string &s) {
		node *it = find_incomplete(s);

		if (!it) {
			std::cout << s << std::endl;
			return;
		}
		--size_;
		if (!it->leaf()) {
			// It is an internal node which cannot be deleted
			it->unmark();
			return;
		}

		size_t idx = s.length();
		do {
			it = it->parent(); 
			--idx;
		} while (!(it->marked()) && it->get_children().size() == 1);

		it->remove_child(s[idx]);
	}

	size_t size() const {
		return size_;
	}

private:
	std::vector<string> suggestions_impl(const node &n, const string &s) const {
		std::vector<string> results;

		if (n.leaf()) {
			return results;
		}

		string curr;
		for (const auto &node : n.get_children()) {
			curr = s + node.value();
			if (node.marked()) results.push_back(curr);
			if (n.leaf()) continue;
			auto children_res = suggestions_impl(node, curr);
			results.reserve(results.size() + children_res.size());
			results.insert(end(results), begin(children_res), end(children_res));
		}
		return results;
	}

#ifdef EXPERIMENTAL_CORO
	std::experimental::generator<string> lazy_suggestions_impl(const node &n, const string &s) const {
		if (n.leaf())
			co_return;

		string curr;
		for (const auto &node : n.get_children()) {
			curr = s + node.value();
			
			if (node.marked()) co_yield curr;
			if (n.leaf()) continue;

			for (auto &str : lazy_suggestions_impl(node,curr)) {
				co_yield std::move(str);
			}
		}

		co_return;
	}
#endif
private:
	node root_{nullptr, '\0', true};
	size_t size_{ 0 };

};

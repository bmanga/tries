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
template <class ValueT, class DepthT, class ValueTraits>
class node_t 
{
	template <class Traits>
	struct node_less {
		bool operator () (const node_t &lhs, const node_t &rhs) const {
			return Traits::lt(lhs.value(), rhs.value());
		}
		bool operator () (const node_t &lhs, const ValueT &rhs) const {
			return Traits::lt(lhs.value(), rhs);
		}
		bool operator () (const ValueT &lhs, const node_t &rhs) const {
			return Traits::lt(lhs, rhs.value());
		}
		struct is_transparent {};
	};

public:
	using value_type = ValueT;
	using depth_type = DepthT;
	using traits_type = ValueTraits;
	//using allocator_type = Allocator;

	node_t(node_t *parent, ValueT ch, DepthT depth, bool marked = false) :
		parent_(parent), value_(ch), depth_(depth), height_(0)
	{
		marked_ = marked;
	}

	node_t *emplace_child(ValueT c, bool marked = false) {
		if (!height_) increase_height();
		return const_cast<node_t*>(&*(children_.emplace(this, c, depth_ + 1, marked).first));
	}

	node_t *get_or_emplace(ValueT c) {
		node_t *res = get_child(c);
		if (!res) {
			res = emplace_child(c);
		}

		return res;
	}
	using path_list = std::vector<const node_t *>;

	path_list paths_to(ValueT v, unsigned min_height_req = 0) const {
		path_list results;
		for (const node_t &node : children_) {
			if (const node_t *n = node.get_child(v)) {
				if (n->height_ >= min_height_req) {
					results.push_back(n);
				}
			}
		}

		return results;
	}

	void remove_child(ValueT v) {
		children_.erase(children_.find(v));
		if (children_.size() == 0) {
			decrease_height();
		}
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
		return height_ == 0;
	}

	ValueT value() const {
		return value_;
	}

	node_t *get_child(ValueT ch) {
		auto it = children_.find(ch);

		if (it == end(children_)) return nullptr;

		// I promise I won't change the contained value
		return const_cast<node_t *>(&*it); 
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

	DepthT depth() const {
		return depth_;
	}

private:
	void increase_height() {
		++height_;
		if (parent_ && parent_->height_ <= this->height_) {
			parent_->increase_height();
		}
	}

	void decrease_height() {
		--height_;
		if (parent_ && parent_->height_ >= this->height_) {
			parent_->decrease_height();
		}
	}

private:
	std::set<node_t, node_less<ValueTraits>> children_;
	node_t *parent_;
	ValueT value_;

	DepthT depth_; 
	DepthT height_;
	unsigned marked_ : 1;
	unsigned collapsed_ : 1;
};


template <size_t MaxDepth>
struct depth_t_selector {
	using type = std::conditional_t <
		MaxDepth < 256,
		uint8_t,
		std::conditional_t<
		MaxDepth < 65536,
		uint16_t,
		uint32_t>>;
};
}// namespace impl_


template <class Node>
decltype(auto) node_to_string(const Node *n, size_t extra_entries = 0) {
	std::basic_string<
		typename Node::value_type,
		typename Node::traits_type> result;

	size_t len = n->depth();
	result.reserve(len + extra_entries);
	// back_inserter maybe?
	result.resize(len);
	while (len-- > 0) {
		result[len] = n->value();
		n = n->parent();
	}

	return result;
}



/*****************************************************************************/

template <class CharT = char, 
	size_t MaxNodeDepth = 255,
	class Traits = std::char_traits<CharT>,
	class Allocator = std::allocator<CharT>>
class trie
{
public:
	using node = impl_::node_t<CharT, 
		typename impl_::depth_t_selector<MaxNodeDepth>::type,
		Traits>;

	using string = std::basic_string<CharT, Traits, Allocator>;

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

	node *find_prefix(const string &s, bool closest_match = false) {
		node *current = &root_;
		for (size_t j = 0, len = s.length(); j < len; ++j) {
			node *it = current->get_child(s[j]);
			if (!it) return closest_match ? current : nullptr;
			current = it;
		}

		return current;
	}

	const node *find_prefix(const string &s, bool closest_match = false) const {
		return const_cast<trie *>(this)->find_prefix(s, closest_match);
	}

	node *find_suffix(const node *n, const string &s, bool allow_unmarked = false) {
		if (s.empty()) return n;

		string::const_iterator it = s.cbegin();
		while (it != s.cend()) {
			n = n->get_child(*it);

			if (!n) return nullptr;
		}

		return n->marked() || allow_unmarked ? n : nullptr;
	}

	const node *find_suffix(const node *n, const string &s, bool allow_unmarked = false) const {
		return const_cast<trie *>(this)->find_suffix(n, s, allow_unmarked);
	}

	string suggest_incomplete_fix(const std::string &s) const {

	}

	std::vector<string> complete_suggestions(string s) {
		const node *it = find_prefix(s);
		if (!it) return{};

		auto sug = suggestions_impl(*it, s);
		if (it->marked()) {
			sug.push_back(s);
			std::iter_swap(begin(sug), end(sug) - 1);
		}
		return sug;
	}

	std::vector<string> closest_matches(string s, unsigned changes = 1) {
		const node *it = find_prefix(s, true);
		// Ignore any word whose first letter is incorrect.
		if (!it) return{};

		// Calculate how far away the node is from the actual string length.
		size_t diff = s.size() - it->depth();

		// Trivial case: the found node matches the string.
		if (diff == 0) return { std::move(s) };

		// We give up if the we diverge farther away than the last 2 characters.
		if (diff > 2) return {};

		std::vector<string> results;
		if (diff == 1) {
			for (const auto &child : it->get_children()) {
				if (child.marked()) 
					results.push_back(node_to_string(&child));
			}

			return results;
		}

		// We are two characters behind.
		string last_chars{ s.end() - 2, s.end() - 1 };

		auto paths_to_last = it->paths_to(*(s.end() - 1));

		auto end_it = std::remove_if(begin(paths_to_last),
			end(paths_to_last),
			[](const auto *n) { return !(n->marked()); });

		paths_to_last.erase(end_it, end(paths_to_last));

		results.reserve(paths_to_last.size());
		std::transform(begin(paths_to_last), end(paths_to_last),
			std::back_inserter(results), [](const node *n) { return node_to_string(n); });

		return results;
	}

	std::vector<string> closest_suggestions(string s) {
		const node *it = find_prefix(s);
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
		const node *it = find_prefix(s);
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
		node *it = find_prefix(s);

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
	node root_{nullptr, '\0', 0, true};
	size_t size_{ 0 };
};


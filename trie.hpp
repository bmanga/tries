#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <memory>
#include <type_traits>
#include <string_view>


#ifdef EXPERIMENTAL_CORO
#include <experimental\coroutine>
#include <experimental\generator>
#endif

namespace impl_
{
template <class RecursiveNode, class ValueT, class ValueTraits>
class default_set_storage
{
	template <class Traits>
	struct node_less {
		bool operator () (const RecursiveNode &lhs, const RecursiveNode &rhs) const {
			return Traits::lt(lhs.value(), rhs.value());
		}
		bool operator () (const RecursiveNode &lhs, const ValueT &rhs) const {
			return Traits::lt(lhs.value(), rhs);
		}
		bool operator () (const ValueT &lhs, const RecursiveNode &rhs) const {
			return Traits::lt(lhs, rhs.value());
		}
		struct is_transparent {};
	};
public:
	using node_type = RecursiveNode;
	using entry_type = RecursiveNode;
	using storage_t = std::set<entry_type, node_less<ValueTraits>>;
	using value_type = ValueT;
	using pointer = value_type *;
	using traits = ValueTraits;

	using node_iterator = typename storage_t::iterator;
	using const_node_iterator = typename storage_t::const_iterator;

	node_iterator begin() { return storage_.begin(); }
	node_iterator end() { return storage_.end(); }
protected:
	storage_t storage_;
};

template <class RecursiveNode, class ValueT, class ValueTraits>
class default_vector_storage
{
	struct storage_pair
	{
		template <class... Ts>
		storage_pair(ValueT val, Ts && ...args) :
			value_(val), node_(std::make_unique<RecursiveNode>(val, std::forward<Ts>(args)...))
		{}
		ValueT value() const {
			return value_;
		}

		RecursiveNode *node() {
			return node_.get();
		}

		std::unique_ptr<RecursiveNode> node_;
		ValueT value_;
	};
public:
	using node_type = RecursiveNode;
	using entry_type = storage_pair;
	using storage_t = std::vector<entry_type>;
	using value_type = ValueT;
	using pointer = value_type *;
	using traits = ValueTraits;

	struct node_iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using value_type = node_type;
		using reference = value_type &;
		using pointer = value_type *;
		using storage_it = typename storage_t::iterator;

		node_iterator(storage_it it) : it_(it) {}


		reference operator*() {
			return *it_->node();
		}

		reference operator->() {
			return *it_->node();
		}

		node_iterator &operator++ () {
			++it_;
			return *this;
		}

		bool operator==(const node_iterator &rhs) const {
			return it_ == rhs.it_;
		}

		bool operator!=(const node_iterator &rhs) const {
			return it_ != rhs.it_;
		}
	private:
		storage_it it_;
	};

	node_iterator begin() { return { storage_.begin() }; }
	node_iterator end() { return { storage_.end() }; }
protected:
	storage_t storage_;
};

template <class StorageT>
class default_vector_accessor : private StorageT
{
public:
	using StorageT::begin;
	using StorageT::end;
	using storage_t = typename StorageT::storage_t;
	using value_type = typename StorageT::value_type;
	using traits = typename StorageT::traits;
	using node_type = typename StorageT::node_type;
	using node_pointer = node_type *;
	using pointer = typename StorageT::pointer;
	using entry_type = typename StorageT::entry_type;
	using node_iterator = typename StorageT::node_iterator;

	typename storage_t::iterator find_pos(value_type val) {
		return std::lower_bound(storage_.begin(), storage_.end(), val, [](const entry_type &lhs, value_type rhs) {
			return traits::lt(lhs.value(), rhs);
		});
	}
	template <class... Ts>
	node_iterator emplace(value_type val, Ts && ...args) {
		// Find where the new element should be created.
		auto pos = this->find_pos(val);
		return this->emplace_hint(pos, val, std::forward<Ts>(args)...);
	}

	template <class... Ts>
	node_iterator emplace_hint(typename storage_t::const_iterator pos, value_type val, Ts && ...args) {
		return this->storage_.emplace(pos, val, std::forward<Ts>(args)...);
	}

	node_iterator get(value_type val) {
		auto pos = this->find_pos(val);

		if (pos == storage_.end() || pos->value() != val) return this->end();

		return pos;
	}

	template <class... Ts>
	// Parameter pack contains all the arguments needed for the node constructor
	node_iterator get_or_emplace(value_type val, Ts && ...args) {
		auto pos = this->find_pos(val);
		if (pos == storage_.end() || pos->value() != val) 
			return emplace_hint(pos, std::forward<Ts>(args)...);

		return pos;
	}

	//void remove();

	// FIXME this is a temporary solution for testing purposes only.
	auto get_elements() const{
		return const_cast<default_vector_accessor *>(this)->get_elements();
	}



	struct node_range
	{
		using it = typename storage_t::iterator;
		node_range(node_iterator beg, node_iterator end) : beg_(beg), end_(end) {}

		node_iterator begin() { return beg_; }
		node_iterator end() { return end_; }

		node_iterator beg_;
		node_iterator end_;
	};
	// FIXME this is a temporary solution for testing purposes only.
	auto get_elements() {
		return node_range{ this->begin(), this->end() };
	}

	auto &raw_storage() const {
		return storage_;
	}

};

template <class StorageT>
class unordered_vector_accessor : private StorageT
{
public:
	using StorageT::begin;
	using StorageT::end;
	using storage_t = typename StorageT::storage_t;
	using value_type = typename StorageT::value_type;
	using traits = typename StorageT::traits;
	using node_type = typename StorageT::node_type;
	using node_pointer = node_type *;
	using pointer = typename StorageT::pointer;
	using entry_type = typename StorageT::entry_type;
	using node_iterator = typename StorageT::node_iterator;

	typename storage_t::iterator find_pos(value_type val) {
		return std::find_if(storage_.begin(), storage_.end(), [=](const entry_type &lhs) {
			return traits::eq(lhs.value(), val);
		});
	}

	template <class... Ts>
	node_iterator emplace(Ts && ...args) {
		this->storage_.emplace_back(std::forward<Ts>(args)...);
		return this->storage_.end() - 1;
	}

	node_iterator get(value_type val) {
		return this->find_pos(val);
	}

	template <class... Ts>
	// Parameter pack contains all the arguments needed for the node constructor
	node_iterator get_or_emplace(value_type val, Ts && ...args) {
		auto pos = this->find_pos(val);
		if (pos == storage_.end())
			return emplace(std::forward<Ts>(args)...);

		return pos;
	}

	//void remove();

	// FIXME this is a temporary solution for testing purposes only.
	auto get_elements() const {
		return const_cast<unordered_vector_accessor *>(this)->get_elements();
	}

	struct node_range
	{
		using it = typename storage_t::iterator;
		node_range(node_iterator beg, node_iterator end) : beg_(beg), end_(end) {}

		node_iterator begin() { return beg_; }
		node_iterator end() { return end_; }

		node_iterator beg_;
		node_iterator end_;
	};
	// FIXME this is a temporary solution for testing purposes only.
	auto get_elements() {
		return node_range{ this->begin(), this->end() };
	}



	auto &raw_storage() const {
		return storage_;
	}

};

template <class StorageT>
class default_set_storage_accessor : private StorageT
{
public:
	using StorageT::begin;
	using StorageT::end;
	using storage_t = typename StorageT::storage_t;
	using value_type = typename StorageT::value_type;
	using node_type = typename StorageT::node_type;
	using node_pointer = node_type *;
	using pointer = typename StorageT::pointer;
	using node_iterator = typename StorageT::node_iterator;

	template <class... Ts>
	node_iterator emplace(Ts && ...args) {
		return this->storage_.emplace(std::forward<Ts>(args)...).first;
	}

	node_iterator get(value_type val) {
		return storage_.find(val);
	}

	// We know that for std::set, this is equivalent to an emplace function.
	template <class... Ts>
	node_iterator get_or_emplace(value_type val, Ts && ...args) {
		return emplace(std::forward<Ts>(args)...);
	}

	void remove(value_type val) {

	}

	storage_t &get_elements() {
		return this->storage_;
	}

	const storage_t &get_elements() const {
		return this->storage_;
	}
};

template <
	class ValueT, 
	class DepthT, 
	class ValueTraits, 
	template <class, class, class> class Storage = impl_::default_vector_storage, 
	template<class> class Accessor = impl_::default_vector_accessor>
class node_t : public 
	Accessor<Storage<node_t<ValueT, DepthT, ValueTraits, Storage, Accessor>, ValueT, ValueTraits>>
{
	using StorageT_ = Storage<node_t, ValueT, ValueTraits>;
	using AccessorT_ = Accessor<Storage<node_t, ValueT, ValueTraits>>;

public:
	using value_type = ValueT;
	using depth_type = DepthT;
	using traits_type = ValueTraits;
	//using iterator = typename AccessorT_::child_iterator;
	//using allocator_type = Allocator;

	node_t(ValueT ch, node_t *parent, DepthT depth, bool marked = false) :
		parent_(parent), value_(ch), depth_(depth), height_(0)
	{
		marked_ = marked;
	}

	node_t *emplace_child(ValueT c, bool marked = false) {
		if (height_ == 0) increase_height();
		return this->AccessorT_::emplace(c, this, depth_ + 1, marked);
	}

	node_t *get_or_emplace(ValueT c) {
		if (height_ == 0) increase_height();
		return mut_ptr_cast_(std::addressof(*this->AccessorT_::get_or_emplace(c, c, this, depth_ + 1, false)));
	}

	using path_list = std::vector<const node_t *>;

	path_list paths_to(ValueT v, unsigned min_height_req = 0) const {
		path_list results;
		for (auto &node : get_elements()) {

			if (const node_t *n = node.get_child(v)) {
				if (n->height_ >= min_height_req) {
					results.push_back(n);
				}
			}
		}

		return results;
	}

	void append_paths_to(path_list &results, ValueT v, unsigned min_height_req = 0) const {
		for (const node_t &node : this->AccessorT::get_elements()) {
			if (const node_t *n = node.get_child(v)) {
				if (n->height_ >= min_height_req) {
					results.push_back(n);
				}
			}
		}

		return results;
	}


	path_list paths_to2(ValueT v, unsigned min_height_req = 0) const {
		path_list results;
		for (const node_t &node : this->AccessorT::get_elements()) {
			node.append_paths_to(results, v, min_height_req);

		}

		return results;
	}




	//void remove_child(ValueT v) {
	//	children_.erase(children_.find(v));
	//	if (children_.size() == 0) {
	//		decrease_height();
	//	}
	//}

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
		auto it = this->AccessorT_::get(ch);
		if (it == this->end()) return nullptr;
		return mut_ptr_cast_(std::addressof(*this->AccessorT_::get(ch)));
	}

	node_t *mut_ptr_cast_(const node_t *node) {
		return const_cast<node_t *>(node);
	}

	const node_t *get_child(ValueT ch) const {
		return const_cast<node_t *>(this)->get_child(ch);
	}
	const auto &get_children() const {
		return const_cast<node_t *>(this)->AccessorT_::raw_storage();
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
		MaxDepth <= std::numeric_limits<uint8_t>::max(),
		uint8_t,
		std::conditional_t<
		MaxDepth <= std::numeric_limits<uint16_t>::max(),
		uint16_t,
		uint32_t>>;
};
}// namespace impl_

namespace utils {
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

} // namespace utils

/*****************************************************************************/

template <
	class CharT = char, 
	size_t MaxNodeDepth = 255,
	class Traits = std::char_traits<CharT>,
	template <class, class, class> class Storage = impl_::default_vector_storage, 
	template <class> class Accessor = impl_::default_vector_accessor>
class trie
{
public:
	using node = impl_::node_t<CharT, 
		typename impl_::depth_t_selector<MaxNodeDepth>::type,
		Traits, Storage, Accessor>;

	using string = std::basic_string<CharT, Traits>;
	using string_view = std::basic_string_view<CharT, Traits>;

	void add(string_view s) {
		node *current = &root_;
		for (size_t j = 0, len = s.length(); j < len; ++j) {
			current = current->get_or_emplace(s[j]);
		}
		if (!current->marked()) {
			++size_;
			current->mark();
		}
	}

	node *find_prefix(string_view s, bool closest_match = false) {
		node *current = &root_;
		for (size_t j = 0, len = s.length(); j < len; ++j) {
			node *it = current->get_child(s[j]);
			if (!it) return closest_match ? current : nullptr;
			current = it;
		}

		return current;
	}

	const node *find_prefix(string_view s, bool closest_match = false) const {
		return const_cast<trie *>(this)->find_prefix(s, closest_match);
	}

	node *find_suffix(const node *n, string_view s, bool allow_unmarked = false) {
		if (s.empty()) return n;

		auto it = s.cbegin();
		while (it != s.cend()) {
			n = n->get_child(*it);

			if (!n) return nullptr;
		}

		return n->marked() || allow_unmarked ? n : nullptr;
	}

	const node *find_suffix(const node *n, string_view s, bool allow_unmarked = false) const {
		return const_cast<trie *>(this)->find_suffix(n, s, allow_unmarked);
	}

	string suggest_incomplete_fix(string_view s) const {

	}

	std::vector<string> complete_suggestions(string_view s) {
		const node *it = find_prefix(s);
		if (!it) return{};

		auto sug = suggestions_impl(*it, s);
		if (it->marked()) {
			sug.emplace_back(s);
			std::iter_swap(begin(sug), end(sug) - 1);
		}
		return sug;
	}

	std::vector<string> closest_matches(string_view s, unsigned changes = 1) {
		const node *it = find_prefix(s, true);
		// Ignore any word whose first letter is incorrect.
		if (!it) return{};

		// Calculate how far away the node is from the actual string length.
		size_t diff = s.size() - it->depth();

		// Trivial case: the found node matches the string.
		if (diff == 0) return { string{s} };

		// We give up if the we diverge farther away than the last 2 characters.
		if (diff > 2) return {};

		std::vector<string> results;
		if (diff == 1) {
			for (const node &child : it->get_elements()) {
				if (child.marked()) 
					results.push_back(utils::node_to_string(&child));
			}

			return results;
		}

		auto paths_to_last = it->paths_to(*(s.end() - 1));
		results.reserve(paths_to_last.size());

		for (const node *n : paths_to_last) {
			if (n->marked()) 
				results.push_back(utils::node_to_string(n));
		}

		return results;
	}

	std::vector<string> closest_suggestions(string_view s) {
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
	
	void remove(string_view s) {
		node *it = find_prefix(s);

		if (!it) {
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

//FIXME		it->remove_child(s[idx]);
	}

	size_t size() const {
		return size_;
	}

private:
	std::vector<string> suggestions_impl(const node &n, string_view s) const {
		std::vector<string> results;

		if (n.leaf()) {
			return results;
		}

		string curr;
		for (const auto &node : n.get_elements()) {
			curr = s;
			curr += node.value();
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
	node root_{'\0', nullptr, 0, true};
	size_t size_{ 0 };
};


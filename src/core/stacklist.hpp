#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include <qlist.h>
#include <qtypes.h>

template <class T, size_t N>
class StackList {
public:
	T& operator[](size_t i) {
		if (i < N) {
			return this->array[i];
		} else {
			return this->vec[i - N];
		}
	}

	const T& operator[](size_t i) const {
		return const_cast<StackList<T, N>*>(this)->operator[](i); // NOLINT
	}

	void push(const T& value) {
		if (this->size < N) {
			this->array[this->size] = value;
		} else {
			this->vec.push_back(value);
		}

		++this->size;
	}

	[[nodiscard]] size_t length() const { return this->size; }
	[[nodiscard]] bool isEmpty() const { return this->size == 0; }

	[[nodiscard]] bool operator==(const StackList<T, N>& other) const {
		if (other.size != this->size) return false;

		for (size_t i = 0; i != this->size; ++i) {
			if (this->operator[](i) != other[i]) return false;
		}

		return true;
	}

	[[nodiscard]] QList<T> toList() const {
		QList<T> list;
		list.reserve(this->size);

		for (const auto& entry: *this) {
			list.push_back(entry);
		}

		return list;
	}

	template <typename Self, typename ListPtr, typename IT>
	struct BaseIterator {
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = int64_t;
		using value_type = IT;
		using pointer = IT*;
		using reference = IT&;

		BaseIterator() = default;
		explicit BaseIterator(ListPtr list, size_t i): list(list), i(i) {}

		reference operator*() const { return this->list->operator[](this->i); }
		pointer operator->() const { return &**this; }

		Self& operator++() {
			++this->i;
			return *static_cast<Self*>(this);
		}

		Self& operator--() {
			--this->i;
			return *static_cast<Self*>(this);
		}

		Self operator++(int) {
			auto v = *this;
			this->operator++();
			return v;
		}

		Self operator--(int) {
			auto v = *this;
			this->operator--();
			return v;
		}

		difference_type operator-(const Self& other) {
			return static_cast<int64_t>(this->i) - static_cast<int64_t>(other.i);
		}

		Self& operator+(difference_type offset) {
			return Self(this->list, static_cast<int64_t>(this->i) + offset);
		}

		[[nodiscard]] bool operator==(const Self& other) const {
			return this->list == other.list && this->i == other.i;
		}

		[[nodiscard]] bool operator!=(const Self& other) const { return !(*this == other); }

	private:
		ListPtr list = nullptr;
		size_t i = 0;
	};

	struct Iterator: public BaseIterator<Iterator, StackList<T, N>*, T> {
		Iterator() = default;
		Iterator(StackList<T, N>* list, size_t i)
		    : BaseIterator<Iterator, StackList<T, N>*, T>(list, i) {}
	};

	struct ConstIterator: public BaseIterator<ConstIterator, const StackList<T, N>*, const T> {
		ConstIterator() = default;
		ConstIterator(const StackList<T, N>* list, size_t i)
		    : BaseIterator<ConstIterator, const StackList<T, N>*, const T>(list, i) {}
	};

	[[nodiscard]] Iterator begin() { return Iterator(this, 0); }
	[[nodiscard]] Iterator end() { return Iterator(this, this->size); }

	[[nodiscard]] ConstIterator begin() const { return ConstIterator(this, 0); }
	[[nodiscard]] ConstIterator end() const { return ConstIterator(this, this->size); }

	[[nodiscard]] bool isContiguous() const { return this->vec.empty(); }
	[[nodiscard]] const T* pArray() const { return this->array.data(); }
	[[nodiscard]] size_t dataLength() const { return this->size * sizeof(T); }

	const T* populateAlloc(void* alloc) const {
		auto arraylen = std::min(this->size, N) * sizeof(T);
		memcpy(alloc, this->array.data(), arraylen);

		if (!this->vec.empty()) {
			memcpy(
			    static_cast<char*>(alloc) + arraylen, // NOLINT
			    this->vec.data(),
			    this->vec.size() * sizeof(T)
			);
		}

		return static_cast<T*>(alloc);
	}

private:
	std::array<T, N> array {};
	std::vector<T> vec;
	size_t size = 0;
};

// might be incorrectly aligned depending on type
// #define STACKLIST_ALLOCA_VIEW(list) ((list).isContiguous() ? (list).pArray() : (list).populateAlloc(alloca((list).dataLength())))

// NOLINTBEGIN
#define STACKLIST_VLA_VIEW(type, list, var)                                                        \
	const type* var;                                                                                 \
	type var##Data[(list).length()];                                                                 \
	if ((list).isContiguous()) {                                                                     \
		(var) = (list).pArray();                                                                       \
	} else {                                                                                         \
		(list).populateAlloc(var##Data);                                                               \
		(var) = var##Data;                                                                             \
	}
// NOLINTEND

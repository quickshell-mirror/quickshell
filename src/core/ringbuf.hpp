#pragma once

#include <new>
#include <tuple>
#include <utility>

#include <qcontainerfwd.h>
#include <qhashfunctions.h>
#include <qtclasshelpermacros.h>
#include <qtypes.h>

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

// capacity 0 buffer cannot be inserted into, only replaced with =
// this is NOT exception safe for constructors
template <typename T>
class RingBuffer {
public:
	explicit RingBuffer() = default;
	explicit RingBuffer(qsizetype capacity): mCapacity(capacity) {
		if (capacity > 0) this->createData();
	}

	~RingBuffer() { this->deleteData(); }

	Q_DISABLE_COPY(RingBuffer);

	explicit RingBuffer(RingBuffer&& other) noexcept { *this = std::move(other); }

	RingBuffer& operator=(RingBuffer&& other) noexcept {
		this->deleteData();
		this->data = other.data;
		this->head = other.head;
		this->mSize = other.mSize;
		this->mCapacity = other.mCapacity;
		other.data = nullptr;
		other.head = -1;
		return *this;
	}

	// undefined if capacity is 0
	template <typename... Args>
	T& emplace(Args&&... args) {
		auto i = (this->head + 1) % this->mCapacity;

		if (this->indexIsAllocated(i)) {
			this->data[i].~T();
		}

		auto* slot = &this->data[i];
		new (&this->data[i]) T(std::forward<Args>(args)...);

		this->head = i;
		if (this->mSize != this->mCapacity) this->mSize = i + 1;

		return *slot;
	}

	void clear() {
		if (this->head == -1) return;

		auto i = this->head;

		do {
			i = (i + 1) % this->mSize;
			this->data[i].~T();
		} while (i != this->head);

		this->mSize = 0;
		this->head = -1;
	}

	// negative indexes and >size indexes are undefined
	[[nodiscard]] T& at(qsizetype i) {
		auto bufferI = (this->head - i) % this->mCapacity;
		if (bufferI < 0) bufferI += this->mCapacity;
		return this->data[bufferI];
	}

	[[nodiscard]] const T& at(qsizetype i) const {
		return const_cast<RingBuffer<T>*>(this)->at(i); // NOLINT
	}

	[[nodiscard]] qsizetype size() const { return this->mSize; }
	[[nodiscard]] qsizetype capacity() const { return this->mCapacity; }

private:
	void createData() {
		if (this->data != nullptr) return;
		this->data =
		    static_cast<T*>(::operator new(this->mCapacity * sizeof(T), std::align_val_t {alignof(T)}));
	}

	void deleteData() {
		this->clear();
		::operator delete(this->data, std::align_val_t {alignof(T)});
		this->data = nullptr;
	}

	bool indexIsAllocated(qsizetype index) {
		return this->mSize == this->mCapacity || index <= this->head;
	}

	T* data = nullptr;
	qsizetype mCapacity = 0;
	qsizetype head = -1;
	qsizetype mSize = 0;
};

// ring buffer with the ability to look up elements by hash (single bucket)
template <typename T>
class HashBuffer {
public:
	explicit HashBuffer() = default;
	explicit HashBuffer(qsizetype capacity): ring(capacity) {}
	~HashBuffer() = default;

	Q_DISABLE_COPY(HashBuffer);
	explicit HashBuffer(HashBuffer&& other) noexcept: ring(other.ring) {}

	HashBuffer& operator=(HashBuffer&& other) noexcept {
		this->ring = other.ring;
		return *this;
	}

	// returns the index of the given value or -1 if missing
	[[nodiscard]] qsizetype indexOf(const T& value, T** slot = nullptr) {
		auto hash = qHash(value);

		for (auto i = 0; i < this->size(); i++) {
			auto& v = this->ring.at(i);
			if (hash == v.first && value == v.second) {
				if (slot != nullptr) *slot = &v.second;
				return i;
			}
		}

		return -1;
	}

	[[nodiscard]] qsizetype indexOf(const T& value, T const** slot = nullptr) const {
		return const_cast<HashBuffer<T>*>(this)->indexOf(value, slot); // NOLINT
	}

	template <typename... Args>
	T& emplace(Args&&... args) {
		auto& entry = this->ring.emplace(
		    std::piecewise_construct,
		    std::forward_as_tuple(0),
		    std::forward_as_tuple(std::forward<Args>(args)...)
		);

		entry.first = qHash(entry.second);
		return entry.second;
	}

	void clear() { this->ring.clear(); }

	// negative indexes and >size indexes are undefined
	[[nodiscard]] T& at(qsizetype i) { return this->ring.at(i).second; }
	[[nodiscard]] const T& at(qsizetype i) const { return this->ring.at(i).second; }
	[[nodiscard]] qsizetype size() const { return this->ring.size(); }
	[[nodiscard]] qsizetype capacity() const { return this->ring.capacity(); }

private:
	RingBuffer<std::pair<size_t, T>> ring;
};

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

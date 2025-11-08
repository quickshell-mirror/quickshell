#pragma once

#include <glib-object.h>

namespace qs::service::polkit {

struct GObjectNoRefTag {};
constexpr GObjectNoRefTag G_OBJECT_NO_REF;

template <typename T>
class GObjectRef {
public:
	explicit GObjectRef(T* ptr = nullptr): ptr(ptr) {
		if (this->ptr) {
			g_object_ref(this->ptr);
		}
	}

	explicit GObjectRef(T* ptr, GObjectNoRefTag /*tag*/): ptr(ptr) {}

	~GObjectRef() {
		if (this->ptr) {
			g_object_unref(this->ptr);
		}
	}

	// We do handle self-assignment in a more general case by checking the
	// included pointers rather than the wrapper objects themselves.
	// NOLINTBEGIN(bugprone-unhandled-self-assignment)

	GObjectRef(const GObjectRef& other): GObjectRef(other.ptr) {}
	GObjectRef& operator=(const GObjectRef& other) {
		if (*this == other) return *this;
		if (this->ptr) {
			g_object_unref(this->ptr);
		}
		this->ptr = other.ptr;
		if (this->ptr) {
			g_object_ref(this->ptr);
		}
		return *this;
	}

	GObjectRef(GObjectRef&& other) noexcept: ptr(other.ptr) { other.ptr = nullptr; }
	GObjectRef& operator=(GObjectRef&& other) noexcept {
		if (*this == other) return *this;
		if (this->ptr) {
			g_object_unref(this->ptr);
		}
		this->ptr = other.ptr;
		other.ptr = nullptr;
		return *this;
	}

	// NOLINTEND(bugprone-unhandled-self-assignment)

	[[nodiscard]] T* get() const { return this->ptr; }
	T* operator->() const { return this->ptr; }

	bool operator==(const GObjectRef<T>& other) const { return this->ptr == other.ptr; }

private:
	T* ptr;
};
} // namespace qs::service::polkit
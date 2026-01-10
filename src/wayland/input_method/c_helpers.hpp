#pragma once

#include <memory>

#include <sys/mman.h>

namespace qs::wayland::input_method::impl {

struct FreeDeleter {
	void operator()(const char* p) const;
};
// Dont use this for literals, only c strings that were allocated
using uniqueCString = std::unique_ptr<const char, FreeDeleter>;

// this is a bit half baked but works for what I need it for
/// Handles the lifetime of a memory mapped shm
class SharedMemory {
public:
	SharedMemory(const char* shmName, int oFlag, size_t size);
	~SharedMemory();
	SharedMemory(const SharedMemory&) = delete;
	SharedMemory(SharedMemory&& other) noexcept;
	SharedMemory& operator=(const SharedMemory&) = delete;
	SharedMemory& operator=(SharedMemory&& other) noexcept;

	[[nodiscard]] operator bool() const;
	[[nodiscard]] int get() const;

	void write(const char* string);

private:
	const char* mShmName;
	size_t mSize;
	int fd;
	char* map = nullptr;
};

} // namespace qs::wayland::input_method::impl

#include "c_helpers.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <utility>

#include <qdebug.h>
#include <qlogging.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>

namespace qs::wayland::input_method::impl {

void FreeDeleter::operator()(const char* p) const {
	std::free(const_cast<std::remove_const_t<char>*>(p)); // NOLINT
}

SharedMemory::SharedMemory(const char* shmName, int oFlag, size_t size)
    : mShmName(shmName)
    , mSize(size)
    , fd(shm_open(this->mShmName, oFlag, 0))
    , map(nullptr) {
	if (this->fd == -1) {
		perror("");
		qDebug() << "Virtual keyboard failed to open shared memory";
		return;
	}
	if (ftruncate(this->fd, static_cast<int>(size)) == -1) {
		this->fd = -1;
		perror("");
		qDebug() << "Virtual keyboard failed to resize shared memory to" << size;
		return;
	}
	this->map = static_cast<char*>(mmap(nullptr, this->mSize, PROT_WRITE, MAP_SHARED, this->fd, 0));
	if (this->map == MAP_FAILED) {
		perror("");
		qDebug() << "Virtual keyboard failed to open shared memory";
		return;
	}
}
SharedMemory::~SharedMemory() {
	if (this->fd != -1) {
		close(this->fd);
		shm_unlink(this->mShmName);
	}
	if (this->map != nullptr) {
		munmap(this->map, this->mSize);
	}
}
SharedMemory::SharedMemory(SharedMemory&& other) noexcept
    : mShmName(std::exchange(other.mShmName, nullptr))
    , mSize(std::exchange(other.mSize, 0))
    , fd(std::exchange(other.fd, -1))
    , map(std::exchange(other.map, nullptr)) {}
SharedMemory& SharedMemory::operator=(SharedMemory&& other) noexcept {
	this->mShmName = std::exchange(other.mShmName, nullptr);
	this->mSize = std::exchange(other.mSize, 0);
	this->fd = std::exchange(other.fd, -1);
	this->map = std::exchange(other.map, nullptr);
	return *this;
}

SharedMemory::operator bool() const { return fd != -1 && map != MAP_FAILED; }
[[nodiscard]] int SharedMemory::get() const { return fd; }

void SharedMemory::write(const char* string) {
	if (!this->map) return;
	strcpy(this->map, string);
}

} // namespace qs::wayland::input_method::impl

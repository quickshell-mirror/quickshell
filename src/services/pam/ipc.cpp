#include "ipc.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

#include <unistd.h>

PamIpcPipes::~PamIpcPipes() {
	if (this->fdIn != 0) close(this->fdIn);
	if (this->fdOut != 0) close(this->fdOut);
}

bool PamIpcPipes::readBytes(char* buffer, size_t length) const {
	size_t i = 0;

	while (i < length) {
		auto count = read(this->fdIn, buffer + i, length - i); // NOLINT

		if (count == -1 || count == 0) {
			return false;
		}

		i += count;
	}

	return true;
}

bool PamIpcPipes::writeBytes(const char* buffer, size_t length) const {
	size_t i = 0;
	while (i < length) {
		auto count = write(this->fdOut, buffer + i, length - i); // NOLINT

		if (count == -1 || count == 0) {
			return false;
		}

		i += count;
	}

	return true;
}

std::string PamIpcPipes::readString(bool* ok) const {
	if (ok != nullptr) *ok = false;

	uint32_t length = 0;
	if (!this->readBytes(reinterpret_cast<char*>(&length), sizeof(length))) {
		return "";
	}

	char data[length]; // NOLINT
	if (!this->readBytes(data, length)) {
		return "";
	}

	if (ok != nullptr) *ok = true;

	return std::string(data, length);
}

bool PamIpcPipes::writeString(const std::string& string) const {
	uint32_t length = string.length();
	if (!this->writeBytes(reinterpret_cast<char*>(&length), sizeof(length))) {
		return false;
	}

	return this->writeBytes(string.data(), string.length());
}

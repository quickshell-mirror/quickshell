#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <qtclasshelpermacros.h>

enum class PamIpcEvent : uint8_t {
	Request,
	Exit,
};

enum class PamIpcExitCode : uint8_t {
	Success,
	StartFailed,
	AuthFailed,
	MaxTries,
	PamError,
	OtherError,
};

struct PamIpcRequestFlags {
	bool echo;
	bool error;
	bool responseRequired;
};

class PamIpcPipes {
public:
	explicit PamIpcPipes() = default;
	explicit PamIpcPipes(int fdIn, int fdOut): fdIn(fdIn), fdOut(fdOut) {}
	~PamIpcPipes();
	Q_DISABLE_COPY_MOVE(PamIpcPipes);

	[[nodiscard]] bool readBytes(char* buffer, size_t length) const;
	[[nodiscard]] bool writeBytes(const char* buffer, size_t length) const;
	[[nodiscard]] std::string readString(bool* ok) const;
	[[nodiscard]] bool writeString(const std::string& string) const;

	int fdIn = 0;
	int fdOut = 0;
};

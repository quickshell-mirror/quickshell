#pragma once

#include <cmath>
#include <functional>
#include <limits>
#include <utility>
#include <variant>

#include <qflags.h>
#include <qlocalserver.h>
#include <qlocalsocket.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/logcat.hpp"

template <typename... Types>
constexpr void assertSerializable() {
	// monostate being zero ensures transactional reads wont break
	static_assert(
	    std::is_same_v<std::variant_alternative_t<0, std::variant<Types...>>, std::monostate>,
	    "Serialization of variants without std::monostate at index 0 is disallowed."
	);

	static_assert(
	    sizeof...(Types) <= std::numeric_limits<quint8>::max(),
	    "Serialization of variants that can't fit the tag in a uint8 is disallowed."
	);
}

template <typename... Types>
QDataStream& operator<<(QDataStream& stream, const std::variant<Types...>& variant) {
	assertSerializable<Types...>();

	if (variant.valueless_by_exception()) {
		stream << static_cast<quint8>(0); // must be monostate
	} else {
		stream << static_cast<quint8>(variant.index());
		std::visit([&]<typename T>(const T& value) { stream << value; }, variant);
	}

	return stream;
}

template <typename... Types>
constexpr bool forEachTypeIndex(const auto& f) {
	return [&]<size_t... Index>(std::index_sequence<Index...>) {
		return (f(std::in_place_index_t<Index>()) || ...);
	}(std::index_sequence_for<Types...>());
}

template <typename... Types>
std::variant<Types...> createIndexedOrMonostate(size_t index, std::variant<Types...>& variant) {
	assertSerializable<Types...>();

	const auto initialized =
	    forEachTypeIndex<Types...>([index, &variant]<size_t Index>(std::in_place_index_t<Index>) {
		    if (index == Index) {
			    variant.template emplace<Index>();
			    return true;
		    } else {
			    return false;
		    }
	    });

	if (!initialized) {
		variant = std::monostate();
	}

	return variant;
}

template <typename... Types>
QDataStream& operator>>(QDataStream& stream, std::variant<Types...>& variant) {
	assertSerializable<Types...>();

	quint8 index = 0;
	stream >> index;

	createIndexedOrMonostate<Types...>(index, variant);
	std::visit([&]<typename T>(T& value) { stream >> value; }, variant);

	return stream;
}

template <typename... Types>
QDataStream& streamInValues(QDataStream& stream, const Types&... types) {
	return (stream << ... << types);
}

template <typename... Types>
QDataStream& streamOutValues(QDataStream& stream, Types&... types) {
	return (stream >> ... >> types);
}

// NOLINTBEGIN
#define DEFINE_SIMPLE_DATASTREAM_OPS(Type, ...)                                                    \
	inline QDataStream& operator<<(QDataStream& stream, const Type& __VA_OPT__(data)) {              \
		return streamInValues(stream __VA_OPT__(, __VA_ARGS__));                                       \
	}                                                                                                \
                                                                                                   \
	inline QDataStream& operator>>(QDataStream& stream, Type& __VA_OPT__(data)) {                    \
		return streamOutValues(stream __VA_OPT__(, __VA_ARGS__));                                      \
	}
// NOLINTEND

DEFINE_SIMPLE_DATASTREAM_OPS(std::monostate);

namespace qs::ipc {

QS_DECLARE_LOGGING_CATEGORY(logIpc);

template <typename T>
class MessageStream {
public:
	explicit MessageStream(QDataStream* stream, QLocalSocket* socket)
	    : stream(stream)
	    , socket(socket) {}

	template <typename V>
	MessageStream& operator<<(V value) {
		*this->stream << T(value);
		this->socket->flush();
		return *this;
	}

private:
	QDataStream* stream;
	QLocalSocket* socket;
};

class IpcServer: public QObject {
	Q_OBJECT;

public:
	explicit IpcServer(const QString& path);

	static void start();

private slots:
	void onNewConnection();

private:
	QLocalServer server;
};

class IpcServerConnection: public QObject {
	Q_OBJECT;

public:
	explicit IpcServerConnection(QLocalSocket* socket, IpcServer* server);

	template <typename T>
	void respond(const T& message) {
		this->stream << message;
		this->socket->flush();
	}

	template <typename T>
	MessageStream<T> responseStream() {
		return MessageStream<T>(&this->stream, this->socket);
	}

	// public for access by nonlocal handlers
	QLocalSocket* socket;
	QDataStream stream;

private slots:
	void onDisconnected();
	void onReadyRead();
};

class IpcClient: public QObject {
	Q_OBJECT;

public:
	explicit IpcClient(const QString& path);

	[[nodiscard]] bool isConnected() const;
	void waitForConnected();
	void waitForDisconnected();

	void kill();

	template <typename T>
	void sendMessage(const T& message) {
		this->stream << message;
		this->socket.flush();
	}

	template <typename T>
	bool waitForResponse(T& slot) {
		while (this->socket.waitForReadyRead(-1)) {
			this->stream.startTransaction();
			this->stream >> slot;
			if (!this->stream.commitTransaction()) continue;
			return true;
		}

		qCCritical(logIpc) << "Error occurred while waiting for response.";
		return false;
	}

	[[nodiscard]] static int
	connect(const QString& id, const std::function<void(IpcClient& client)>& callback);

	// public for access by nonlocal handlers
	QLocalSocket socket;
	QDataStream stream;

signals:
	void connected();
	void disconnected();

private slots:
	static void onError(QLocalSocket::LocalSocketError error);
};

} // namespace qs::ipc

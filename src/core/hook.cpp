#include <dlfcn.h>

#include <array>
#include <cstdlib>
#include <string>

#include <cpptrace/basic.hpp>

#include <QCoreApplication>
#include <QEvent>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QObject>

__attribute__((constructor)) static void initHook() {
	unsetenv("LD_PRELOAD");
}

Q_LOGGING_CATEGORY(logDestructors, "quickshell.destructors");
Q_LOGGING_CATEGORY(logPostEvent, "quickshell.postEvent");

static std::string traceAddrs() {
	std::array<cpptrace::frame_ptr, 64> frames{};
	auto count = cpptrace::safe_generate_raw_trace(frames.data(), frames.size(), 1);
	std::string result;
	for (size_t i = 0; i < count; i++) {
		if (i > 0) result += ',';
		char buf[20];
		snprintf(buf, sizeof(buf), "%p", reinterpret_cast<void*>(frames[i]));
		result += buf;
	}
	return result;
}

using VoidMemberFn = void (*)(QObject*);

static VoidMemberFn realD2() {
	static auto fn = reinterpret_cast<VoidMemberFn>(dlsym(RTLD_NEXT, "_ZN7QObjectD2Ev"));
	return fn;
}

static VoidMemberFn realDeleteLater() {
	static auto fn =
	    reinterpret_cast<VoidMemberFn>(dlsym(RTLD_NEXT, "_ZN7QObject11deleteLaterEv"));
	return fn;
}

using PostEventFn = void (*)(QObject*, QEvent*, int);

static PostEventFn realPostEvent() {
	static auto fn = reinterpret_cast<PostEventFn>(
	    dlsym(RTLD_NEXT, "_ZN16QCoreApplication9postEventEP7QObjectP6QEventi")
	);
	return fn;
}

extern "C" {

void _ZN7QObjectD2Ev(QObject* self) {
	const auto* meta = self->metaObject();
	auto trace = traceAddrs();
	qCDebug(logDestructors, "~QObject %p %s\n%s", static_cast<void*>(self), meta->className(), trace.c_str());
	realD2()(self);
}

void _ZN7QObject11deleteLaterEv(QObject* self) {
	const auto* meta = self->metaObject();
	auto trace = traceAddrs();
	qCDebug(logDestructors, "deleteLater %p %s\n%s", static_cast<void*>(self), meta->className(), trace.c_str());
	realDeleteLater()(self);
}

void _ZN16QCoreApplication9postEventEP7QObjectP6QEventi(
    QObject* receiver,
    QEvent* event,
    int priority
) {
	//if (event->type() == QEvent::User + 1) {
		const auto* meta = receiver->metaObject();
		auto trace = traceAddrs();
		qCDebug(logPostEvent, "%p %d %s\n%s", static_cast<void*>(receiver), event->type(), meta->className(), trace.c_str());
	//}
	realPostEvent()(receiver, event, priority);
}

} // extern "C"

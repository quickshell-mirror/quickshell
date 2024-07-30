#pragma once

// NOLINTBEGIN
#define DROP_EMIT(object, func)                                                                    \
	DropEmitter(object, static_cast<void (*)(typeof(object))>([](typeof(object) o) { o->func(); }))

#define DROP_EMIT_IF(cond, object, func) (cond) ? DROP_EMIT(object, func) : DropEmitter()

#define DEFINE_DROP_EMIT_IF(cond, object, func) DropEmitter func = DROP_EMIT_IF(cond, object, func)

#define DROP_EMIT_SET(object, local, member, signal)                                               \
	auto signal = DropEmitter();                                                                     \
	if (local == object->member) {                                                                   \
		object->member = local;                                                                        \
		signal = DROP_EMIT(object, signal);                                                            \
	}

// generic accessor declarations

#define GDECL_GETTER(type, name) [[nodiscard]] type name() const

#define GDEF_GETTER(class, type, member, name)                                                     \
	type class::name() const { return this->member; }

#define GDECL_SETTER(type, name) DropEmitter name(type value)

#define GDEF_SETTER(class, type, member, name, signal)                                             \
	DropEmitter class ::name(type value) {                                                           \
		if (value == this->member) return DropEmitter();                                               \
		this->member = value;                                                                          \
		return DROP_EMIT(this, signal);                                                                \
	}

#define GDECL_MEMBER(type, getter, setter)                                                         \
	GDECL_GETTER(type, getter)                                                             \
	GDECL_SETTER(type, setter)

#define GDEF_MEMBER(class, type, member, getter, setter, signal)                                   \
	GDEF_GETTER(class, type, member, getter)                                               \
	GDEF_SETTER(class, type, member, setter, signal)

#define GDEF_MEMBER_S(class, type, lower, upper)                                         \
	GDEF_MEMBER(class, type, m##upper, lower, set##upper, lower##Changed)

// NOLINTEND

class DropEmitter {
public:
	Q_DISABLE_COPY(DropEmitter);
	template <class O>
	DropEmitter(O* object, void (*signal)(O*))
	    : object(object)
	    , signal(*reinterpret_cast<void (*)(void*)>(signal)) {} // NOLINT

	DropEmitter() = default;

	DropEmitter(DropEmitter&& other) noexcept: object(other.object), signal(other.signal) {
		other.object = nullptr;
	}

	~DropEmitter() { this->call(); }

	DropEmitter& operator=(DropEmitter&& other) noexcept {
		this->object = other.object;
		this->signal = other.signal;
		other.object = nullptr;
		return *this;
	}

	explicit operator bool() const noexcept { return this->object; }

	void call() {
		if (!this->object) return;
		this->signal(this->object);
		this->object = nullptr;
	}

private:
	void* object = nullptr;
	void (*signal)(void*) = nullptr;
};

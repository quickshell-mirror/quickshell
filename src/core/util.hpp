#pragma once
#include <algorithm>
#include <type_traits>

#include <bit>
#include <qlatin1stringview.h>
#include <qobject.h>
#include <qproperty.h>
#include <qstringview.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

template <size_t length>
struct StringLiteral {
	constexpr StringLiteral(const char (&str)[length]) { // NOLINT
		std::copy_n(str, length, this->value);
	}

	constexpr operator const char*() const noexcept { return this->value; }
	operator QLatin1StringView() const { return QLatin1String(this->value, length); }

	char value[length]; // NOLINT
};

template <size_t length>
struct StringLiteral16 {
	constexpr StringLiteral16(const char16_t (&str)[length]) { // NOLINT
		std::copy_n(str, length, this->value);
	}

	[[nodiscard]] constexpr const QChar* qCharPtr() const noexcept {
		return std::bit_cast<const QChar*>(&this->value); // NOLINT
	}

	[[nodiscard]] Q_ALWAYS_INLINE operator QString() const noexcept {
		return QString::fromRawData(this->qCharPtr(), static_cast<qsizetype>(length - 1));
	}

	[[nodiscard]] Q_ALWAYS_INLINE operator QStringView() const noexcept {
		return QStringView(this->qCharPtr(), static_cast<qsizetype>(length - 1));
	}

	char16_t value[length]; // NOLINT
};

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
// NOLINTEND

class DropEmitter {
public:
	Q_DISABLE_COPY(DropEmitter);
	template <class O>
	DropEmitter(O* object, void (*signal)(O*))
	    : object(object)
	    , signal(*reinterpret_cast<void (*)(void*)>(signal)) {}

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

	// orders calls for multiple emitters (instead of reverse definition order)
	template <typename... Args>
	static void call(Args&... args) {
		(args.call(), ...);
	}

private:
	void* object = nullptr;
	void (*signal)(void*) = nullptr;
};

// NOLINTBEGIN
#define DECLARE_MEMBER(class, name, member, signal)                                                \
	using M_##name = MemberMetadata<&class ::member, &class ::signal>

#define DECLARE_MEMBER_NS(class, name, member) using M_##name = MemberMetadata<&class ::member>

#define DECLARE_MEMBER_GET(name) [[nodiscard]] M_##name::Ref name() const
#define DECLARE_MEMBER_SET(name, setter) M_##name::Ret setter(M_##name::Ref value)

#define DECLARE_MEMBER_GETSET(name, setter)                                                        \
	DECLARE_MEMBER_GET(name);                                                                        \
	DECLARE_MEMBER_SET(name, setter)

#define DECLARE_MEMBER_SETONLY(class, name, setter, member, signal) DECLARE_MEMBER(cl

#define DECLARE_MEMBER_FULL(class, name, setter, member, signal)                                   \
	DECLARE_MEMBER(class, name, member, signal);                                                     \
	DECLARE_MEMBER_GETSET(name, setter)

#define DECLARE_MEMBER_WITH_GET(class, name, member, signal)                                       \
	DECLARE_MEMBER(class, name, member, signal);                                                     \
                                                                                                   \
public:                                                                                            \
	DECLARE_MEMBER_GET(name);                                                                        \
                                                                                                   \
private:

#define DECLARE_PRIVATE_MEMBER(class, name, setter, member, signal)                                \
	DECLARE_MEMBER_WITH_GET(class, name, member, signal);                                            \
	DECLARE_MEMBER_SET(name, setter);

#define DECLARE_PMEMBER(type, name) using M_##name = PseudomemberMetadata<type, true>;
#define DECLARE_PMEMBER_NS(type, name) using M_##name = PseudomemberMetadata<type, false>;

#define DECLARE_PMEMBER_FULL(type, name, setter)                                                   \
	DECLARE_PMEMBER(type, name);                                                                     \
	DECLARE_MEMBER_GETSET(name, setter)

#define DECLARE_PMEMBER_WITH_GET(type, name)                                                       \
	DECLARE_PMEMBER(type, name);                                                                     \
                                                                                                   \
public:                                                                                            \
	DECLARE_MEMBER_GET(name);                                                                        \
                                                                                                   \
private:

#define DECLARE_PRIVATE_PMEMBER(type, name, setter)                                                \
	DECLARE_PMEMBER_WITH_GET(type, name);                                                            \
	DECLARE_MEMBER_SET(name, setter);

#define DEFINE_PMEMBER_GET_M(Class, Member, name) Member::Ref Class::name() const
#define DEFINE_PMEMBER_GET(Class, name) DEFINE_PMEMBER_GET_M(Class, Class::M_##name, name)

#define DEFINE_MEMBER_GET_M(Class, Member, name)                                                   \
	DEFINE_PMEMBER_GET_M(Class, Member, name) { return Member::get(this); }

#define DEFINE_MEMBER_GET(Class, name) DEFINE_MEMBER_GET_M(Class, Class::M_##name, name)

#define DEFINE_MEMBER_SET_M(Class, Member, setter)                                                 \
	Member::Ret Class::setter(Member::Ref value) { return Member::set(this, value); }

#define DEFINE_MEMBER_SET(Class, name, setter) DEFINE_MEMBER_SET_M(Class, Class::M_##name, setter)

#define DEFINE_MEMBER_GETSET(Class, name, setter)                                                  \
	DEFINE_MEMBER_GET(Class, name)                                                                   \
	DEFINE_MEMBER_SET(Class, name, setter)

#define MEMBER_EMIT(name) std::remove_reference_t<decltype(*this)>::M_##name::emitter(this)
// NOLINTEND

template <typename T>
struct MemberPointerTraits;

template <typename T, typename C>
struct MemberPointerTraits<T C::*> {
	using Class = C;
	using Type = T;
};

template <auto member, auto signal = nullptr>
class MemberMetadata {
	using Traits = MemberPointerTraits<decltype(member)>;
	using Class = Traits::Class;

public:
	using Type = Traits::Type;
	using Ref = const Type&;
	using Ret = std::conditional_t<std::is_null_pointer_v<decltype(signal)>, void, DropEmitter>;

	static Ref get(const Class* obj) { return obj->*member; }

	static Ret set(Class* obj, Ref value) {
		if constexpr (signal == nullptr) {
			if (MemberMetadata::get(obj) == value) return;
			obj->*member = value;
		} else {
			if (MemberMetadata::get(obj) == value) return DropEmitter();
			obj->*member = value;
			return MemberMetadata::emitter(obj);
		}
	}

	static Ret emitter(Class* obj) {
		if constexpr (signal != nullptr) {
			return DropEmitter(obj, &MemberMetadata::emitForObject);
		}
	}

private:
	static void emitForObject(Class* obj) { (obj->*signal)(); }
};

// allows use of member macros without an actual field backing them
template <typename T, bool hasSignal>
class PseudomemberMetadata {
public:
	using Type = T;
	using Ref = const Type&;
	using Ret = std::conditional_t<hasSignal, DropEmitter, void>;
};

class GuardedEmitBlocker {
public:
	explicit GuardedEmitBlocker(bool* var): var(var) { *this->var = true; }
	~GuardedEmitBlocker() { *this->var = false; }
	Q_DISABLE_COPY_MOVE(GuardedEmitBlocker);

private:
	bool* var;
};

template <auto signal>
class GuardedEmitter {
	using Traits = MemberPointerTraits<decltype(signal)>;
	using Class = Traits::Class;

	bool blocked = false;

public:
	GuardedEmitter() = default;
	~GuardedEmitter() = default;
	Q_DISABLE_COPY_MOVE(GuardedEmitter);

	void call(Class* obj) {
		if (!this->blocked) (obj->*signal)();
	}

	GuardedEmitBlocker block() { return GuardedEmitBlocker(&this->blocked); }
};

template <auto methodPtr>
class MethodFunctor {
	using PtrMeta = MemberPointerTraits<decltype(methodPtr)>;
	using Class = PtrMeta::Class;

public:
	MethodFunctor(Class* obj): obj(obj) {}

	void operator()() { (this->obj->*methodPtr)(); }

private:
	Class* obj;
};

// NOLINTBEGIN
#define QS_BINDING_SUBSCRIBE_METHOD(Class, bindable, method, strategy)                             \
	QPropertyChangeHandler<MethodFunctor<&Class::method>>                                            \
	    _qs_method_subscribe_##bindable##_##method =                                                 \
	        (bindable).strategy(MethodFunctor<&Class::method>(this));
// NOLINTEND

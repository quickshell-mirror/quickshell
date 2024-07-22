#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

class Retainable;

///! Attached object for types that can have delayed destruction.
/// Retainable works as an attached property that allows objects to be
/// kept around (retained) after they would normally be destroyed, which
/// is especially useful for things like exit transitions.
///
/// An object that is retainable will have @@Retainable as an attached property.
/// All retainable objects will say that they are retainable on their respective
/// typeinfo pages.
///
/// > [!INFO] Working directly with @@Retainable is often overly complicated and
/// > error prone. For this reason @@RetainableLock should
/// > usually be used instead.
class RetainableHook: public QObject {
	Q_OBJECT;
	/// If the object is currently in a retained state.
	Q_PROPERTY(bool retained READ isRetained NOTIFY retainedChanged);
	QML_ATTACHED(RetainableHook);
	QML_NAMED_ELEMENT(Retainable);
	QML_UNCREATABLE("Retainable can only be used as an attached object.");

public:
	static RetainableHook* getHook(QObject* object, bool create = false);

	void destroyOnRelease();

	void ref();
	void unref();

	/// Hold a lock on the object so it cannot be destroyed.
	///
	/// A counter is used to ensure you can lock the object from multiple places
	/// and it will not be unlocked until the same number of unlocks as locks have occurred.
	///
	/// > [!WARNING] It is easy to forget to unlock a locked object.
	/// > Doing so will create what is effectively a memory leak.
	/// >
	/// > Using @@RetainableLock is recommended as it will help
	/// > avoid this scenario and make misuse more obvious.
	Q_INVOKABLE void lock();
	/// Remove a lock on the object. See @@lock() for more information.
	Q_INVOKABLE void unlock();
	/// Forcibly remove all locks, destroying the object.
	///
	/// @@unlock() should usually be preferred.
	Q_INVOKABLE void forceUnlock();

	[[nodiscard]] bool isRetained() const;

	static RetainableHook* qmlAttachedProperties(QObject* object);

signals:
	/// This signal is sent when the object would normally be destroyed.
	///
	/// If all signal handlers return and no locks are in place, the object will be destroyed.
	/// If at least one lock is present the object will be retained until all are removed.
	void dropped();
	/// This signal is sent immediately before the object is destroyed.
	/// At this point destruction cannot be interrupted.
	void aboutToDestroy();

	void retainedChanged();

private:
	explicit RetainableHook(QObject* parent): QObject(parent) {}

	void unlocked();

	uint refcount = 0;
	// tracked separately so a warning can be given when unlock is called too many times,
	// without affecting other lock sources such as RetainableLock.
	uint explicitRefcount = 0;
	Retainable* retainableFacet = nullptr;
	bool inactive = true;

	friend class Retainable;
};

class Retainable {
public:
	Retainable() = default;
	virtual ~Retainable() = default;
	Q_DISABLE_COPY_MOVE(Retainable);

	void retainedDestroy();
	[[nodiscard]] bool isRetained() const;

protected:
	virtual void retainFinished();

private:
	RetainableHook* hook = nullptr;
	bool retaining = false;

	friend class RetainableHook;
};

///! A helper for easily using Retainable.
/// A RetainableLock provides extra safety and ease of use for locking
/// @@Retainable objects. A retainable object can be locked by multiple
/// locks at once, and each lock re-exposes relevant properties
/// of the retained objects.
///
/// #### Example
/// The code below will keep a retainable object alive for as long as the
/// RetainableLock exists.
///
/// ```qml
/// RetainableLock {
///   object: aRetainableObject
///   locked: true
/// }
/// ```
class RetainableLock: public QObject {
	Q_OBJECT;
	/// The object to lock. Must be @@Retainable.
	Q_PROPERTY(QObject* object READ object WRITE setObject NOTIFY objectChanged);
	/// If the object should be locked.
	Q_PROPERTY(bool locked READ locked WRITE setLocked NOTIFY lockedChanged);
	/// If the object is currently in a retained state.
	Q_PROPERTY(bool retained READ isRetained NOTIFY retainedChanged);
	QML_ELEMENT;

public:
	explicit RetainableLock(QObject* parent = nullptr): QObject(parent) {}
	~RetainableLock() override;
	Q_DISABLE_COPY_MOVE(RetainableLock);

	[[nodiscard]] QObject* object() const;
	void setObject(QObject* object);

	[[nodiscard]] bool locked() const;
	void setLocked(bool locked);

	[[nodiscard]] bool isRetained() const;

signals:
	/// Rebroadcast of the object's @@Retainable.dropped(s).
	void dropped();
	/// Rebroadcast of the object's @@Retainable.aboutToDestroy(s).
	void aboutToDestroy();
	void retainedChanged();

	void objectChanged();
	void lockedChanged();

private slots:
	void onObjectDestroyed();

private:
	QObject* mObject = nullptr;
	RetainableHook* hook = nullptr;
	bool mEnabled = false;
};

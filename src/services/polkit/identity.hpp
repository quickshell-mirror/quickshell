#pragma once

#include <qobject.h>
#include <qqmlintegration.h>

#include "gobjectref.hpp"

// _PolkitIdentity is considered a reserved identifier, but I am specifically
// forward declaring this reserved name.
using PolkitIdentity = struct _PolkitIdentity; // NOLINT(bugprone-reserved-identifier)

namespace qs::service::polkit {
//! Represents a user or group that can be used to authenticate.
class Identity: public QObject {
	Q_OBJECT;
	Q_DISABLE_COPY_MOVE(Identity);

	// clang-format off
	/// The Id of the identity. If the identity is a user, this is the user's uid. See @@isGroup.
	Q_PROPERTY(quint32 id READ id CONSTANT);

	/// The name of the user or group.
	///
	/// If available, this is the actual username or group name, but may fallback to the ID.
	Q_PROPERTY(QString string READ name CONSTANT);

	/// The full name of the user or group, if available. Otherwise the same as @@name.
	Q_PROPERTY(QString displayName READ displayName CONSTANT);

	/// Indicates if this identity is a group or a user.
	///
	/// If true, @@id is a gid, otherwise it is a uid.
	Q_PROPERTY(bool isGroup READ isGroup CONSTANT);

	QML_UNCREATABLE("Identities cannot be created directly.");
	// clang-format on

public:
	explicit Identity(
	    id_t id,
	    QString name,
	    QString displayName,
	    bool isGroup,
	    GObjectRef<PolkitIdentity> polkitIdentity,
	    QObject* parent = nullptr
	);
	~Identity() override = default;

	static Identity* fromPolkitIdentity(GObjectRef<PolkitIdentity> identity);

	[[nodiscard]] quint32 id() const { return static_cast<quint32>(this->mId); };
	[[nodiscard]] const QString& name() const { return this->mName; };
	[[nodiscard]] const QString& displayName() const { return this->mDisplayName; };
	[[nodiscard]] bool isGroup() const { return this->mIsGroup; };

	GObjectRef<PolkitIdentity> polkitIdentity;

private:
	id_t mId;
	QString mName;
	QString mDisplayName;
	bool mIsGroup;
};
} // namespace qs::service::polkit

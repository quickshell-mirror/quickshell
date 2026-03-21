#include "identity.hpp"
#include <type_traits>
#include <utility>
#include <vector>

#include <qobject.h>
#include <qtmetamacros.h>
#include <sys/types.h>

#define POLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE
// Workaround macro collision with glib 'signals' struct member.
#undef signals
#include <polkit/polkit.h>
#define signals Q_SIGNALS
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include "gobjectref.hpp"

namespace qs::service::polkit {
Identity::Identity(
    id_t id,
    QString name,
    QString displayName,
    bool isGroup,
    GObjectRef<PolkitIdentity> polkitIdentity,
    QObject* parent
)
    : QObject(parent)
    , polkitIdentity(std::move(polkitIdentity))
    , mId(id)
    , mName(std::move(name))
    , mDisplayName(std::move(displayName))
    , mIsGroup(isGroup) {}

Identity* Identity::fromPolkitIdentity(GObjectRef<PolkitIdentity> identity) {
	if (POLKIT_IS_UNIX_USER(identity.get())) {
		auto uid = polkit_unix_user_get_uid(POLKIT_UNIX_USER(identity.get()));

		auto bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
		// The call can fail with -1, in this case choose a default that is
		// big enough.
		if (bufSize == -1) bufSize = 16384;
		auto buffer = std::vector<char>(bufSize);

		std::aligned_storage_t<sizeof(passwd), alignof(passwd)> pwBuf;
		passwd* pw = nullptr;
		getpwuid_r(uid, reinterpret_cast<passwd*>(&pwBuf), buffer.data(), bufSize, &pw);

		auto name =
		    (pw && pw->pw_name && *pw->pw_name) ? QString::fromUtf8(pw->pw_name) : QString::number(uid);

		return new Identity(
		    uid,
		    name,
		    (pw && pw->pw_gecos && *pw->pw_gecos) ? QString::fromUtf8(pw->pw_gecos) : name,
		    false,
		    std::move(identity)
		);
	}

	if (POLKIT_IS_UNIX_GROUP(identity.get())) {
		auto gid = polkit_unix_group_get_gid(POLKIT_UNIX_GROUP(identity.get()));

		auto bufSize = sysconf(_SC_GETGR_R_SIZE_MAX);
		// The call can fail with -1, in this case choose a default that is
		// big enough.
		if (bufSize == -1) bufSize = 16384;
		auto buffer = std::vector<char>(bufSize);

		std::aligned_storage_t<sizeof(group), alignof(group)> grBuf;
		group* gr = nullptr;
		getgrgid_r(gid, reinterpret_cast<group*>(&grBuf), buffer.data(), bufSize, &gr);

		auto name =
		    (gr && gr->gr_name && *gr->gr_name) ? QString::fromUtf8(gr->gr_name) : QString::number(gid);
		return new Identity(gid, name, name, true, std::move(identity));
	}

	// A different type of identity is netgroup.
	return nullptr;
}
} // namespace qs::service::polkit

#pragma once

#include <memory>
#include <optional>

#include <qobject.h>
#include <qregion.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

class QsQuickWindowBase;

namespace qs::window {

class SceneDamageTracker;

class WindowDamageTracker: public QObject {
	Q_OBJECT;

public:
	explicit WindowDamageTracker(QsQuickWindowBase* window);
	~WindowDamageTracker() override;
	Q_DISABLE_COPY_MOVE(WindowDamageTracker);

	// Logical window coordinates; see SceneDamageTracker::windowRect().
	[[nodiscard]] std::optional<QRegion> damage();
	// Geometry still needs preprocess; see SceneDamageTracker::measureGeometry().
	[[nodiscard]] bool pendingPreprocess() const;
	// Damage carried into the next frame by SceneDamageTracker::presented().
	[[nodiscard]] bool pendingDamage() const;
	void beginRender();
	void presented();

private slots:
	void initialize();
	void invalidate();
	void invalidateFrame();
	void sync();
	void afterSync();

private:
	QsQuickWindowBase* window;
	std::unique_ptr<SceneDamageTracker> tracker;
	bool invalidated = false;
	bool externalRendering = false;
	bool loggedExternalRendering = false;
};

} // namespace qs::window

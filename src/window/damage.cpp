#include "damage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <typeinfo>
#include <utility>

#include <private/qquickwindow_p.h>
#include <private/qsgabstractrenderer_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qsgdefaultinternalimagenode_p.h>
#include <private/qsgdefaultinternalrectanglenode_p.h>
#include <private/qsgdistancefieldglyphnode_p_p.h>
#include <private/qsgmaterialshader_p.h>
#include <private/qsgrenderer_p.h>
#include <qcolor.h>
#include <qfile.h>
#include <qhash.h>
#include <qiodevice.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmap.h>
#include <qmatrix4x4.h>
#include <qobject.h>
#include <qpoint.h>
#include <qquickwindow.h>
#include <qrect.h>
#include <qregion.h>
#include <qset.h>
#include <qsgflatcolormaterial.h>
#include <qsggeometry.h>
#include <qsgmaterial.h>
#include <qsgmaterialshader.h>
#include <qsgmaterialtype.h>
#include <qsgnode.h>
#include <qsgrendererinterface.h>
#include <qsgtexturematerial.h>
#include <qsgvertexcolormaterial.h>
#include <qsize.h>
#include <qstring.h>
#include <qtclasshelpermacros.h>
#include <qtypes.h>
#include <qwindow.h>
#include <rhi/qshader.h>
#include <span>

#include "../core/logcat.hpp"
#include "proxywindow.hpp"

namespace qs::window {

namespace {

QS_LOGGING_CATEGORY(logDamage, "quickshell.window.damage", QtWarningMsg);
constexpr qsizetype MAX_DAMAGE_RECTS = 32;

// Glyph shader allowlist for boundedGlyphShader(); windowRect() adds the pixel margin.
constexpr auto GLYPH_SHADERS = std::array {
    ":/qt-project.org/scenegraph/shaders_ng/textmask.vert.qsb",
    ":/qt-project.org/scenegraph/shaders_ng/styledtext.vert.qsb",
    ":/qt-project.org/scenegraph/shaders_ng/outlinedtext.vert.qsb",
    ":/qt-project.org/scenegraph/shaders_ng/distancefieldtext.vert.qsb",
    ":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext.vert.qsb",
    ":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext.vert.qsb",
    ":/qt-project.org/scenegraph/shaders_ng/hiqsubpixeldistancefieldtext.vert.qsb",
    ":/qt-project.org/scenegraph/shaders_ng/loqsubpixeldistancefieldtext.vert.qsb",
};

// Effect shader allowlist for boundedEffectShader().
constexpr auto EFFECT_SHADERS = std::array {
    ":/data/shaders/multieffect_c.vert.qsb",
    ":/data/shaders/multieffect_cs.vert.qsb",
    ":/data/shaders/bluritems.vert.qsb",
    ":/data/shaders/rectangularshadow.vert.qsb",
};

bool isFinite(const QRectF& rect) {
	return std::isfinite(rect.left()) && std::isfinite(rect.top()) && std::isfinite(rect.right())
	    && std::isfinite(rect.bottom());
}

QRegion uniteRects(const QList<QRect>& rects) {
	QRect bounds;
	for (const auto& rect: rects) bounds |= rect;
	if (rects.size() > MAX_DAMAGE_RECTS) return bounds;

	QRegion region;
	for (const auto& rect: rects) {
		auto covers = [&rect](const QRect& other) {
			return other.left() <= rect.left() && other.top() <= rect.top()
			    && other.right() >= rect.right() && other.bottom() >= rect.bottom();
		};

		if (covers(region.boundingRect()) && std::ranges::any_of(region, covers)) continue;
		region += rect;
	}

	return region.rectCount() > MAX_DAMAGE_RECTS ? QRegion(bounds) : region;
}

} // namespace

// Collects damage through nodeChanged(); unrecognized geometry falls back to full damage.
class SceneDamageTracker: public QSGAbstractRenderer {
public:
	SceneDamageTracker() = default;
	~SceneDamageTracker() override { this->setRootNode(nullptr); }
	Q_DISABLE_COPY_MOVE(SceneDamageTracker);

	void setFrame(const QSize& size, qreal scale, const QColor& color, bool full);
	void markFull() { this->full = true; }
	void beginRender() { this->rendering = true; }
	void renderScene() override;
	[[nodiscard]] std::optional<QRegion> damage() const;
	[[nodiscard]] bool pendingPreprocess() const { return this->unmeasured; }
	[[nodiscard]] bool pendingDamage() const { return !this->frameDamage.isEmpty(); }
	void presented();

protected:
	void nodeChanged(QSGNode* node, QSGNode::DirtyState state) override;

private:
	// Cached bounds in parent coordinates; see refresh().
	struct Subtree {
		QRectF rect;
		bool unbounded = false;
		bool antialiased = false;

		Subtree& operator|=(const Subtree& other) {
			if (!other.rect.isEmpty()) this->rect |= other.rect;
			this->unbounded |= other.unbounded;
			this->antialiased |= other.antialiased;
			return *this;
		}
	};

	static constexpr Subtree UNBOUNDED {.rect = {}, .unbounded = true, .antialiased = false};

	struct GlyphAtlas {
		const QSGDistanceFieldTextMaterial* material = nullptr;
		QSize size;
	};

	bool boundedShader(QSGGeometryNode* node, bool glyph, QSGMaterial* material);
	bool boundedGlyphShader(QSGMaterial* material);
	bool boundedEffectShader(QSGMaterial* material);
	void forget(QSGNode* node);
	void forgetChildren(QSGNode* node);
	void addDamage(const QRectF& rect);
	[[nodiscard]] QRectF windowRect(QSGNode* node) const;
	Subtree refresh(QSGNode* node);
	void propagate(QSGNode* node, Subtree subtree);
	Subtree measureGeometry(QSGGeometryNode* node);
	void recordGlyphAtlas(QSGGeometryNode* node);

	std::array<QShader, EFFECT_SHADERS.size()> effectShaders {};
	QHash<QSGMaterialType*, bool> glyphShaders;
	QHash<QSGNode*, Subtree> subtrees;
	QHash<QSGNode*, Subtree> localBounds;
	QSet<QSGNode*> dirty;
	QList<QSGNode*> dirtyNodes;
	QSet<QSGNode*> forgottenDirty;
	QList<QSGNode*> topmost;
	QSet<QSGNode*> stale;
	QSet<QSGNode*> renderNodes;
	QSet<QSGNode*> staleTextures;
	QMap<QSGNode*, GlyphAtlas> glyphAtlases;
	QList<QRect> pending;
	QRegion frameDamage;
	QRegion nextDamage;
	QSize size;
	qreal scale = 1;
	QColor color;
	bool full = true;
	bool unboundedFrame = false;
	bool unmeasured = false;
	bool rendering = false;
	bool loggedEffectShader = false;
};

bool SceneDamageTracker::boundedShader(QSGGeometryNode* node, bool glyph, QSGMaterial* material) {
	if (!material) return false;

	const auto& type = typeid(*material);
	if (type == typeid(QSGFlatColorMaterial) || type == typeid(QSGVertexColorMaterial)
	    || type == typeid(QSGOpaqueTextureMaterial) || type == typeid(QSGTextureMaterial)
	    || type == typeid(QSGSmoothColorMaterial) || type == typeid(QSGSmoothTextureMaterial))
	{
		return true;
	}

	if (glyph) return this->boundedGlyphShader(material);
	if (!dynamic_cast<QSGShaderEffectNode*>(node)) return false;
	// RequiresFullMatrix marks custom vertex shaders; see QSGRhiShaderEffectNode::syncMaterial().
	if (!material->flags().testFlag(QSGMaterial::RequiresFullMatrix)) return true;
	return this->boundedEffectShader(material);
}

bool SceneDamageTracker::boundedGlyphShader(QSGMaterial* material) {
	auto cached = this->glyphShaders.constFind(material->type());
	if (cached != this->glyphShaders.cend()) return *cached;

	auto shader = std::unique_ptr<QSGMaterialShader>(
	    material->createShader(QSGRendererInterface::RenderMode2D)
	);
	auto file =
	    QSGMaterialShaderPrivate::get(shader.get())->shaderFileNames.value(QShader::VertexStage);
	auto bounded =
	    std::ranges::any_of(GLYPH_SHADERS, [&file](const char* path) { return file == path; });
	if (!bounded) qCDebug(logDamage) << "Unrecognized glyph vertex shader:" << file;

	this->glyphShaders.insert(material->type(), bounded);
	return bounded;
}

bool SceneDamageTracker::boundedEffectShader(QSGMaterial* material) {
	auto shader = std::unique_ptr<QSGMaterialShader>(
	    material->createShader(QSGRendererInterface::RenderMode2D)
	);
	auto vertex = QSGMaterialShaderPrivate::get(shader.get())->shader(QShader::VertexStage);
	if (!vertex.isValid()) return false;
	if (std::ranges::find(this->effectShaders, vertex) != this->effectShaders.end()) return true;

	for (size_t i = 0; i < EFFECT_SHADERS.size(); i++) {
		auto& cached = this->effectShaders.at(i);
		if (cached.isValid()) continue;

		auto file = QFile(QString(EFFECT_SHADERS.at(i)));
		if (!file.open(QIODevice::ReadOnly)) continue;
		cached = QShader::fromSerialized(file.readAll());
		if (cached == vertex) return true;
	}

	if (!this->loggedEffectShader) {
		this->loggedEffectShader = true;
		qCDebug(logDamage) << "Unrecognized effect vertex shader; using full damage.";
	}

	return false;
}

void SceneDamageTracker::setFrame(const QSize& size, qreal scale, const QColor& color, bool full) {
	this->full |= full || this->size != size || this->scale != scale || this->color != color;
	this->rendering = false;
	this->size = size;
	this->scale = scale;
	this->color = color;
}

void SceneDamageTracker::nodeChanged(QSGNode* node, QSGNode::DirtyState state) {
	// Capture old bounds now, before an ancestor's transform can change later in the sync.
	if (this->subtrees.contains(node)) this->addDamage(this->windowRect(node));
	if (state.testFlag(QSGNode::DirtyNodeRemoved)) {
		this->forget(node);
		return;
	}

	// Carry layer updates during rendering into the next frame; see QSGRhiLayer::grab().
	if (this->rendering && state.testFlag(QSGNode::DirtyMaterial)
	    && node->flags().testFlag(QSGNode::UsePreprocess))
	{
		this->staleTextures.insert(node);
	}

	// refresh() skips blocked subtrees; discard bounds that may go stale while blocked.
	if (node->isSubtreeBlocked()) this->forgetChildren(node);
	if (this->dirty.contains(node)) return;

	this->dirty.insert(node);
	// forget() leaves the node in dirtyNodes, so don't append it twice.
	if (!this->forgottenDirty.contains(node)) this->dirtyNodes.append(node);
}

void SceneDamageTracker::forget(QSGNode* node) {
	// renderScene() checks dirty before dereferencing entries left in dirtyNodes.
	if (this->dirty.remove(node)) this->forgottenDirty.insert(node);
	this->subtrees.remove(node);
	this->localBounds.remove(node);
	this->renderNodes.remove(node);
	this->staleTextures.remove(node);
	this->glyphAtlases.remove(node);
	this->forgetChildren(node);
}

void SceneDamageTracker::forgetChildren(QSGNode* node) {
	for (auto* child = node->firstChild(); child; child = child->nextSibling()) {
		this->forget(child);
	}
}

void SceneDamageTracker::addDamage(const QRectF& rect) {
	auto aligned = rect.toAlignedRect();
	if (aligned.isEmpty()) return;
	this->pending.append(aligned);
}

QRectF SceneDamageTracker::windowRect(QSGNode* node) const {
	auto subtree = this->subtrees.value(node);
	auto window = QRectF(QPointF(), this->size);
	if (subtree.unbounded) return window;
	if (subtree.rect.isEmpty()) return QRectF();

	QMatrix4x4 matrix;
	for (auto* parent = node->parent(); parent; parent = parent->parent()) {
		if (parent->type() != QSGNode::TransformNodeType) continue;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
		matrix = static_cast<QSGTransformNode*>(parent)->matrix() * matrix;
	}

	if (!matrix.isAffine()) return window;
	auto rect = matrix.mapRect(subtree.rect);
	if (!isFinite(rect)) return window;
	if (!subtree.antialiased) return rect;

	// Include a pixel margin for the antialiased materials selected in measureGeometry().
	auto pixel = 1 / this->scale;
	return rect.adjusted(-pixel, -pixel, pixel, pixel);
}

SceneDamageTracker::Subtree SceneDamageTracker::refresh(QSGNode* node) {
	Subtree subtree;
	if (node->isSubtreeBlocked()) {
		this->subtrees.insert(node, subtree);
		return subtree;
	}

	QMatrix4x4 matrix;
	switch (node->type()) {
	case QSGNode::GeometryNodeType: {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
		auto* geometry = static_cast<QSGGeometryNode*>(node);
		auto local = this->localBounds.constFind(node);
		if (local == this->localBounds.cend() || this->dirty.contains(node)) {
			local = this->localBounds.insert(node, this->measureGeometry(geometry));
			this->recordGlyphAtlas(geometry);
		}

		subtree = *local;
		break;
	}
	case QSGNode::TransformNodeType:
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
		matrix = static_cast<QSGTransformNode*>(node)->matrix();
		subtree.unbounded = !matrix.isAffine();
		break;
	case QSGNode::RenderNodeType:
		this->renderNodes.insert(node);
		subtree.unbounded = true;
		break;
	default: break;
	}

	for (auto* child = node->firstChild(); child; child = child->nextSibling()) {
		auto cached = this->subtrees.constFind(child);
		auto entry = cached == this->subtrees.cend() || this->stale.contains(child)
		               ? this->refresh(child)
		               : *cached;
		entry.rect = matrix.mapRect(entry.rect);
		entry.unbounded |= !isFinite(entry.rect);
		subtree |= entry;
	}

	this->subtrees.insert(node, subtree);
	return subtree;
}

// Cached bounds only grow here; refresh() can shrink them later.
void SceneDamageTracker::propagate(QSGNode* node, Subtree subtree) {
	for (auto* parent = node->parent(); parent; parent = parent->parent()) {
		auto cached = this->subtrees.find(parent);
		if (cached == this->subtrees.end()) return;

		if (parent->type() == QSGNode::TransformNodeType) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
			const auto& matrix = static_cast<QSGTransformNode*>(parent)->matrix();
			subtree.rect = matrix.mapRect(subtree.rect);
			subtree.unbounded |= !matrix.isAffine() || !isFinite(subtree.rect);
		}

		*cached |= subtree;
	}
}

SceneDamageTracker::Subtree SceneDamageTracker::measureGeometry(QSGGeometryNode* node) {
	auto glyph = dynamic_cast<QSGGlyphNode*>(node) != nullptr;
	if (!this->boundedShader(node, glyph, node->material())) return UNBOUNDED;
	if (node->opaqueMaterial() && !this->boundedShader(node, glyph, node->opaqueMaterial())) {
		return UNBOUNDED;
	}

	auto* geometry = node->geometry();
	// Don't skip frames with unfinished glyph geometry; see QSGDefaultGlyphNode::preprocess().
	if (!geometry || geometry->vertexCount() == 0) {
		this->unmeasured |= node->flags().testFlag(QSGNode::UsePreprocess);
		return {};
	}

	if (geometry->attributeCount() == 0) return UNBOUNDED;

	const auto& position = *geometry->attributes();
	if (position.type != QSGGeometry::FloatType || position.tupleSize != 2
	    || !position.isVertexCoordinate)
	{
		return UNBOUNDED;
	}

	auto mode = geometry->drawingMode();
	if (mode != QSGGeometry::DrawTriangles && mode != QSGGeometry::DrawTriangleStrip
	    && mode != QSGGeometry::DrawTriangleFan)
	{
		return UNBOUNDED;
	}

	auto left = std::numeric_limits<float>::max();
	auto top = left;
	auto right = std::numeric_limits<float>::lowest();
	auto bottom = right;
	auto stride = static_cast<size_t>(geometry->sizeOfVertex());
	auto vertices = std::span(
	    static_cast<const char*>(geometry->vertexData()),
	    stride * static_cast<size_t>(geometry->vertexCount())
	);

	for (auto i = 0; i < geometry->vertexCount(); i++) {
		QSGGeometry::Point2D point = {};
		std::memcpy(&point, vertices.data(), sizeof(point));
		if (!std::isfinite(point.x) || !std::isfinite(point.y)) return UNBOUNDED;
		left = std::min(left, point.x);
		top = std::min(top, point.y);
		right = std::max(right, point.x);
		bottom = std::max(bottom, point.y);
		vertices = vertices.subspan(stride);
	}

	auto* material = node->material();
	const auto& type = typeid(*material);
	auto antialiased =
	    glyph || type == typeid(QSGSmoothColorMaterial) || type == typeid(QSGSmoothTextureMaterial);

	auto rect = QRectF(QPointF(left, top), QPointF(right, bottom));
	return {.rect = rect, .unbounded = false, .antialiased = antialiased};
}

void SceneDamageTracker::recordGlyphAtlas(QSGGeometryNode* node) {
	auto* material = dynamic_cast<QSGDistanceFieldTextMaterial*>(node->material());
	if (!material || !material->texture()) {
		this->glyphAtlases.remove(node);
		return;
	}

	this->glyphAtlases.insert(node, {.material = material, .size = material->texture()->size});
}

void SceneDamageTracker::renderScene() {
	// Mark paths below dirty ancestors so refresh() reaches their dirty descendants.
	for (auto* node: std::as_const(this->dirtyNodes)) {
		if (!this->dirty.contains(node)) continue;

		auto* ancestor = node->parent();
		while (ancestor && !this->dirty.contains(ancestor) && !this->stale.contains(ancestor)
		       && !ancestor->isSubtreeBlocked())
		{
			ancestor = ancestor->parent();
		}

		if (!ancestor) {
			this->topmost.append(node);
			continue;
		}

		if (ancestor->isSubtreeBlocked()) continue;
		for (auto* path = node; path != ancestor; path = path->parent()) this->stale.insert(path);
	}

	for (auto* node: std::as_const(this->topmost)) {
		this->propagate(node, this->refresh(node));
		this->addDamage(this->windowRect(node));
	}

	this->dirty.clear();
	this->dirtyNodes.clear();
	this->forgottenDirty.clear();
	this->topmost.clear();
	this->stale.clear();

	// Track atlas resizing separately from node dirtiness; see QSGRhiDistanceFieldGlyphCache::resizeTexture().
	for (auto [node, atlas]: this->glyphAtlases.asKeyValueRange()) {
		const auto& size = atlas.material->texture()->size;
		if (size == atlas.size) continue;
		atlas.size = size;
		this->addDamage(this->windowRect(node));
	}

	this->frameDamage += uniteRects(this->pending);
	this->pending.clear();

	if (this->rendering) {
		for (auto* node: std::as_const(this->staleTextures)) {
			auto rect = this->windowRect(node).toAlignedRect();
			if (!rect.isEmpty()) this->nextDamage += rect;
		}

		this->staleTextures.clear();
		this->rendering = false;
	}
	// Keep full damage for render nodes even without a nodeChanged() notification.
	this->unboundedFrame = !this->renderNodes.isEmpty();
}

std::optional<QRegion> SceneDamageTracker::damage() const {
	if (this->full || this->unboundedFrame) return std::nullopt;
	if (this->frameDamage.rectCount() > MAX_DAMAGE_RECTS) {
		return QRegion(this->frameDamage.boundingRect());
	}

	return this->frameDamage;
}

void SceneDamageTracker::presented() {
	this->frameDamage = std::exchange(this->nextDamage, QRegion());
	this->full = false;
	this->unmeasured = false;
}

WindowDamageTracker::WindowDamageTracker(QsQuickWindowBase* window)
    : QObject(window)
    , window(window) {
	// clang-format off
	QObject::connect(window, &QQuickWindow::sceneGraphInitialized, this, &WindowDamageTracker::initialize);
	QObject::connect(window, &QQuickWindow::sceneGraphInvalidated, this, &WindowDamageTracker::invalidate);
	// Force a full frame when shown again; see sync().
	QObject::connect(window, &QWindow::visibleChanged, this, &WindowDamageTracker::invalidateFrame);
	// clang-format on
}

WindowDamageTracker::~WindowDamageTracker() { this->invalidate(); }

void WindowDamageTracker::initialize() {
	this->tracker = std::make_unique<SceneDamageTracker>();

	// clang-format off
	QObject::connect(this->window, &QQuickWindow::beforeSynchronizing, this, &WindowDamageTracker::sync);
	QObject::connect(this->window, &QQuickWindow::afterSynchronizing, this, &WindowDamageTracker::afterSync);
	// clang-format on
}

void WindowDamageTracker::invalidate() {
	// clang-format off
	QObject::disconnect(this->window, &QQuickWindow::beforeSynchronizing, this, &WindowDamageTracker::sync);
	QObject::disconnect(this->window, &QQuickWindow::afterSynchronizing, this, &WindowDamageTracker::afterSync);
	// clang-format on

	this->tracker.reset();
}

void WindowDamageTracker::invalidateFrame() { this->invalidated = true; }

void WindowDamageTracker::sync() {
	// Read pendingFontUpdate before QQuickWindowPrivate::syncSceneGraph() clears it.
	auto* priv = QQuickWindowPrivate::get(this->window);
	this->tracker->setFrame(
	    this->window->size(),
	    this->window->effectiveDevicePixelRatio(),
	    this->window->color(),
	    std::exchange(this->invalidated, false) || priv->pendingFontUpdate
	        || !this->window->frameMargins().isNull() || !priv->visualizationMode.isEmpty()
	);
}

void WindowDamageTracker::afterSync() {
	// QQuickWindowPrivate::syncSceneGraph() creates the root before afterSynchronizing.
	this->tracker->setRootNode(QQuickWindowPrivate::get(this->window)->renderer->rootNode());
}

bool WindowDamageTracker::pendingPreprocess() const {
	return this->tracker && this->tracker->pendingPreprocess();
}

bool WindowDamageTracker::pendingDamage() const {
	return this->tracker && this->tracker->pendingDamage();
}

void WindowDamageTracker::beginRender() {
	if (this->tracker) this->tracker->beginRender();
}

std::optional<QRegion> WindowDamageTracker::damage() {
	if (!this->tracker) return std::nullopt;
	this->tracker->renderScene();

	// Keep full damage for one frame after external rendering stops.
	auto external = this->window->hasRenderReceivers();
	if (this->externalRendering && !external) this->tracker->markFull();
	this->externalRendering = external;

	if (external && !this->loggedExternalRendering) {
		this->loggedExternalRendering = true;
		qCDebug(logDamage) << "Window has render signal receivers; using full damage.";
	}

	if (external) return std::nullopt;
	return this->tracker->damage();
}

void WindowDamageTracker::presented() {
	if (this->tracker) this->tracker->presented();
}

} // namespace qs::window

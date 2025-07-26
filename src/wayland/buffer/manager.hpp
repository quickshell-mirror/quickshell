#pragma once

#include <cstdint>
#include <memory>

#include <qhash.h>
#include <qlist.h>
#include <qmatrix4x4.h>
#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qvariant.h>
#include <sys/types.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "../../core/stacklist.hpp"

class QQuickWindow;

namespace qs::wayland::buffer {

class WlBufferManagerPrivate;
class WlBufferQSGTexture;

struct WlBufferTransform {
	enum Transform : uint8_t {
		Normal0 = 0,
		Normal90 = 1,
		Normal180 = 2,
		Normal270 = 3,
		Flipped0 = 4,
		Flipped90 = 5,
		Flipped180 = 6,
		Flipped270 = 7,
	} transform = Normal0;

	WlBufferTransform() = default;
	WlBufferTransform(uint8_t transform): transform(static_cast<Transform>(transform)) {}

	[[nodiscard]] int degrees() const { return 90 * (this->transform & 0b11111011); }
	[[nodiscard]] bool flip() const { return this->transform & 0b00000100; }
	[[nodiscard]] bool flipSize() const { return this->transform & 0b00000001; }

	void apply(QMatrix4x4& matrix) const {
		matrix.rotate(this->flip() ? 180 : 0, 0, 1, 0);
		matrix.rotate(static_cast<float>(this->degrees()), 0, 0, 1);
	}
};

struct WlBufferRequest {
	uint32_t width = 0;
	uint32_t height = 0;

	struct DmaFormat {
		DmaFormat() = default;
		DmaFormat(uint32_t format): format(format) {}

		uint32_t format = 0;
		StackList<uint64_t, 10> modifiers;
	};

	struct {
		StackList<uint32_t, 1> formats;
	} shm;

	struct {
		dev_t device = 0;
		StackList<DmaFormat, 1> formats;
	} dmabuf;

	void reset();
};

class WlBuffer {
public:
	virtual ~WlBuffer() = default;
	Q_DISABLE_COPY_MOVE(WlBuffer);

	[[nodiscard]] virtual wl_buffer* buffer() const = 0;
	[[nodiscard]] virtual QSize size() const = 0;
	[[nodiscard]] virtual bool isCompatible(const WlBufferRequest& request) const = 0;
	[[nodiscard]] operator bool() const { return this->buffer(); }

	// Must be called from render thread.
	[[nodiscard]] virtual WlBufferQSGTexture* createQsgTexture(QQuickWindow* window) const = 0;

	WlBufferTransform transform;

protected:
	explicit WlBuffer() = default;
};

class WlBufferSwapchain {
public:
	[[nodiscard]] WlBuffer*
	createBackbuffer(const WlBufferRequest& request, bool* newBuffer = nullptr);

	void swapBuffers() { this->presentSecondBuffer = !this->presentSecondBuffer; }

	[[nodiscard]] WlBuffer* backbuffer() const {
		return this->presentSecondBuffer ? this->buffer1.get() : this->buffer2.get();
	}

	[[nodiscard]] WlBuffer* frontbuffer() const {
		return this->presentSecondBuffer ? this->buffer2.get() : this->buffer1.get();
	}

private:
	std::unique_ptr<WlBuffer> buffer1;
	std::unique_ptr<WlBuffer> buffer2;
	bool presentSecondBuffer = false;

	friend class WlBufferQSGDisplayNode;
};

class WlBufferManager: public QObject {
	Q_OBJECT;

public:
	~WlBufferManager() override;
	Q_DISABLE_COPY_MOVE(WlBufferManager);

	static WlBufferManager* instance();

	[[nodiscard]] bool isReady() const;
	[[nodiscard]] WlBuffer* createBuffer(const WlBufferRequest& request);

signals:
	void ready();

private:
	explicit WlBufferManager();

	WlBufferManagerPrivate* p;
};

} // namespace qs::wayland::buffer

#include "types.hpp"

#include <qdebug.h>
#include <qnamespace.h>
#include <qrect.h>

QRect Box::qrect() const { return {this->x, this->y, this->w, this->h}; }

bool Box::operator==(const Box& other) const {
	return this->x == other.x && this->y == other.y && this->w == other.w && this->h == other.h;
}

QDebug operator<<(QDebug debug, const Box& box) {
	auto saver = QDebugStateSaver(debug);
	debug.nospace() << "Box(" << box.x << ',' << box.y << ' ' << box.w << 'x' << box.h << ')';
	return debug;
}

Qt::Edges Edges::toQt(Edges::Flags edges) { return Qt::Edges(edges.toInt()); }

bool Edges::isOpposing(Edges::Flags edges) {
	return edges.testFlags(Edges::Top | Edges::Bottom) || edges.testFlags(Edges::Left | Edges::Right);
}

DropEmitter::DropEmitter(DropEmitter&& other) noexcept: object(other.object), signal(other.signal) {
	other.object = nullptr;
}

DropEmitter& DropEmitter::operator=(DropEmitter&& other) noexcept {
	this->object = other.object;
	this->signal = other.signal;
	other.object = nullptr;
	return *this;
}

DropEmitter::~DropEmitter() { this->call(); }

void DropEmitter::call() {
	if (!this->object) return;
	this->signal(this->object);
	this->object = nullptr;
}

#include "util.hpp"

#include <qbytearray.h>
#include <qguiapplication.h>
#include <qguiapplication_platform.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

xcb_connection_t* x11Connection() {
	static xcb_connection_t* conn = nullptr; // NOLINT

	if (conn == nullptr) {
		if (auto* x11Application = dynamic_cast<QGuiApplication*>(QGuiApplication::instance())
		                               ->nativeInterface<QNativeInterface::QX11Application>())
		{
			conn = x11Application->connection();
		}
	}

	return conn;
}

// NOLINTBEGIN
XAtom XAtom::_NET_WM_STRUT {};
XAtom XAtom::_NET_WM_STRUT_PARTIAL {};
XAtom XAtom::_NET_WM_DESKTOP {};
// NOLINTEND

void XAtom::initAtoms() {
	_NET_WM_STRUT.init("_NET_WM_STRUT");
	_NET_WM_STRUT_PARTIAL.init("_NET_WM_STRUT_PARTIAL");
	_NET_WM_DESKTOP.init("_NET_WM_DESKTOP");
}

void XAtom::init(const QByteArray& name) {
	this->cookie = xcb_intern_atom(x11Connection(), 0, name.length(), name.data());
}

bool XAtom::isValid() {
	this->resolve();
	return this->mAtom != XCB_ATOM_NONE;
}

const xcb_atom_t& XAtom::atom() {
	this->resolve();
	return this->mAtom;
}

void XAtom::resolve() {
	if (!this->resolved) {
		this->resolved = true;

		auto* reply = xcb_intern_atom_reply(x11Connection(), this->cookie, nullptr);
		if (reply != nullptr) this->mAtom = reply->atom;
		free(reply); // NOLINT
	}
}

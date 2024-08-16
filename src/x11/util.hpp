#pragma once

#include <qbytearray.h>
#include <qtclasshelpermacros.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

xcb_connection_t* x11Connection();

class XAtom {
public:
	[[nodiscard]] bool isValid();
	[[nodiscard]] const xcb_atom_t& atom();

	// NOLINTBEGIN
	static XAtom _NET_WM_STRUT;
	static XAtom _NET_WM_STRUT_PARTIAL;
	static XAtom _NET_WM_DESKTOP;
	// NOLINTEND

	static void initAtoms();

private:
	void init(const QByteArray& name);
	void resolve();

	bool resolved = false;
	xcb_atom_t mAtom = XCB_ATOM_NONE;
	xcb_intern_atom_cookie_t cookie {};
};

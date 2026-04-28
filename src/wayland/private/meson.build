protos = {
	'xdg-shell': wayland.find_protocol('xdg-shell'),
	'ext-background-effect-v1': wayland.find_protocol(
		'ext-background-effect',
		state: 'staging',
		version: 1,
	),
	'idle-inhibit-unstable-v1': wayland.find_protocol(
		'idle-inhibit',
		state: 'unstable',
		version: 1,
	),
	'ext-idle-notify-v1': wayland.find_protocol(
		'ext-idle-notify',
		state: 'staging',
		version: 1,
	),
	'keyboard-shortcuts-inhibit-unstable-v1': wayland.find_protocol(
		'keyboard-shortcuts-inhibit',
		state: 'unstable',
		version: 1,
	),
	'ext-workspace-v1': wayland.find_protocol(
		'ext-workspace',
		state: 'staging',
		version: 1,
	),
	'ext-session-lock-v1': wayland.find_protocol(
		'ext-session-lock',
		state: 'staging',
		version: 1,
	),
	'wlr-layer-shell-unstable-v1': '../wlr_layershell/wlr-layer-shell-unstable-v1.xml',
	'wlr-foreign-toplevel-management-unstable-v1': '../toplevel_management/wlr-foreign-toplevel-management-unstable-v1.xml',
	'ext-foreign-toplevel-list-v1': wayland.find_protocol(
		'ext-foreign-toplevel-list',
		state: 'staging',
		version: 1,
	),
	'ext-image-copy-capture-v1': wayland.find_protocol(
		'ext-image-copy-capture',
		state: 'staging',
		version: 1,
	),
	'ext-image-capture-source-v1': wayland.find_protocol(
		'ext-image-capture-source',
		state: 'staging',
		version: 1,
	),
	'wlr-screencopy-unstable-v1': '../screencopy/wlr_screencopy/wlr-screencopy-unstable-v1.xml',
	'hyprland-toplevel-export-v1': '../screencopy/hyprland_screencopy/hyprland-toplevel-export-v1.xml',
	'hyprland-toplevel-mapping-v1': '../hyprland/ipc/hyprland-toplevel-mapping-v1.xml',
	'hyprland-focus-grab-v1': '../hyprland/focus_grab/hyprland-focus-grab-v1.xml',
	'hyprland-global-shortcuts-v1': '../hyprland/global_shortcuts/hyprland-global-shortcuts-v1.xml',
	'hyprland-surface-v1': '../hyprland/surface/hyprland-surface-v1.xml',
	'linux-dmabuf-v1': wayland.find_protocol('linux-dmabuf', version: 1),
}

proto_link = {}
qs_wl_inc += include_directories('.')

foreach name, xml : protos
	# cannot use this because qtwaylandscanner expects a hardcoded file name prefix :(
	# wayland.scan_xml(xml)

	proto_c_h = custom_target(
		name + '-c-header',
		input: [xml],
		output: 'wayland-@0@-client-protocol.h'.format(name),
		command: [waylandscanner, 'client-header', '@INPUT@', '@OUTPUT@'],
	)
	proto_qt_h = custom_target(
		name + '-qt-header',
		input: [xml],
		output: 'qwayland-@0@.h'.format(name),
		command: [qtwaylandscanner, 'client-header', '@INPUT@'],
		capture: true,
	)

	proto_c = custom_target(
		name + '-c-code',
		input: [xml],
		output: 'wayland-@0@-client-protocol.c'.format(name),
		command: [waylandscanner, 'private-code', '@INPUT@', '@OUTPUT@'],
		depends: proto_c_h,
	)
	proto_qt = custom_target(
		name + '-qt-code',
		input: [xml],
		output: 'qwayland-@0@.cpp'.format(name),
		command: [qtwaylandscanner, 'client-code', '@INPUT@', '--header-path=.'],
		capture: true,
		depends: [proto_qt_h, proto_c_h],
	)

	proto_link += {
		name: static_library(
			name + '-lib',
			proto_c,
			proto_qt,
			dependencies: [
				dep_qt6wayland,
				dep_wl_client,
			],
		),
	}
endforeach

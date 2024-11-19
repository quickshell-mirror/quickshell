# Adds a dependency hint to the link order, but does not block build on the dependency.
function (qs_add_link_dependencies target)
	set_property(
		TARGET ${target}
		APPEND PROPERTY INTERFACE_LINK_LIBRARIES
		${ARGN}
	)
endfunction()

function (qs_append_qmldir target text)
	get_property(qmldir_content TARGET ${target} PROPERTY _qt_internal_qmldir_content)

	if ("${qmldir_content}" STREQUAL "")
		message(WARNING "qs_append_qmldir depends on private Qt cmake code, which has broken.")
		return()
	endif()

	set_property(TARGET ${target} APPEND_STRING PROPERTY _qt_internal_qmldir_content ${text})
endfunction()

# DEPENDENCIES introduces a cmake dependency which we don't need with static modules.
# This greatly improves comp speed by not introducing those dependencies.
function (qs_add_module_deps_light target)
	foreach (dep IN LISTS ARGN)
		string(APPEND qmldir_extra "depends ${dep}\n")
	endforeach()

	qs_append_qmldir(${target} "${qmldir_extra}")
endfunction()

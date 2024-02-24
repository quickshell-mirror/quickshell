builddir := 'build'

with_cpphpp := 'find src -type f \( -name "*.cpp" -o -name "*.hpp" \) -print0 | xargs -0'
with_cpp := 'find src -type f -name "*.cpp" -print0 | xargs -0'

fmt:
    {{with_cpphpp}} clang-format -i

lint:
    {{with_cpp}} clang-tidy --load={{ env_var("TIDYFOX") }}

fixlint:
    {{with_cpp}} clang-tidy --load={{ env_var("TIDYFOX") }}

configure target='debug' *FLAGS='':
	cmake -B {{builddir}} \
		-DCMAKE_BUILD_TYPE={{ if target == "debug" { "Debug" } else { "Release" } }} \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON {{FLAGS}}

	ln -sf {{builddir}}/compile_commands.json compile_commands.json

_configure_if_clean:
	@if ! [ -d {{builddir}} ]; then just configure; fi

build: _configure_if_clean
	cmake --build {{builddir}}

clean:
	rm -f compile_commands.json
	rm -rf {{builddir}}

run *ARGS='': build
	{{builddir}}/src/core/quickshell {{ARGS}}

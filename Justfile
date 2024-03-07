builddir := 'build'

fmt:
    find src -type f \( -name "*.cpp" -o -name "*.hpp" \) -print0 | xargs -0 clang-format -i

lint:
	find src -type f -name "*.cpp" -print0 | parallel -q0 --eta clang-tidy --load={{ env_var("TIDYFOX") }}

configure target='debug' *FLAGS='':
	cmake -GNinja -B {{builddir}} \
		-DCMAKE_BUILD_TYPE={{ if target == "debug" { "Debug" } else { "RelWithDebInfo" } }} \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON {{FLAGS}}

	ln -sf {{builddir}}/compile_commands.json compile_commands.json

_configure_if_clean:
	@if ! [ -d {{builddir}} ]; then just configure; fi

build: _configure_if_clean
	cmake --build {{builddir}}

release: (configure "release") build

clean:
	rm -f compile_commands.json
	rm -rf {{builddir}}

run *ARGS='': build
	{{builddir}}/src/core/quickshell {{ARGS}}

test *ARGS='': build
	ctest --test-dir {{builddir}} --output-on-failure {{ARGS}}

install *ARGS='':
	cmake --install {{builddir}} {{ARGS}}

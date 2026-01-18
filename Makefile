.PHONY: all clean test run

all:
	mkdir -p build
	cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	make -C build
	# Expose compile_commands.json at project root for tools (clangd, bear, etc.)
	@if [ -f build/compile_commands.json ]; then \
		ln -sf $(CURDIR)/build/compile_commands.json $(CURDIR)/compile_commands.json; \
	fi

clean:
	rm -rf build

run: all
	./build/meal_prep

test: all
	cd build && ctest --output-on-failure

integration-test: all
	./build/meal_prep -m turkey-burgers -m turkey-meatballs
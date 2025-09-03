# Top-level makefile loosely based on https://github.com/embeddedartistry/cmake-project-skeleton/blob/main/Makefile
# you can set this to 1 to see all commands that are being run
VERBOSE ?= 0

ifeq ($(VERBOSE),1)
export Q :=
export VERBOSE := 1
else
export Q := @
export VERBOSE := 0
endif

.PHONY: obliterate-all confirm-obliterate
confirm-obliterate:
	$(Q) echo "This will remove the entire build structure and may include externals!"
	$(Q) echo "Are you sure? [y/N]" && read ans && [ $${ans:-N} = y ]

obliterate-all: confirm-obliterate
	$(Q) rm -rf build
#	$(Q) rm -rf external

######
## Wren build commands
.PHONY: cmake build tests test run-tests benchmark metrics clean obliterate
cmake:
	$(Q) cmake -B build

build/Makefile:
	$(Q) echo "Wren has not been configured, please run 'make cmake' first"
	$(Q) exit 1

build: build/Makefile
	$(Q) make -C build wren_test

tests: build/Makefile
	$(Q) make -C build tests

run-tests: build/Makefile
	$(Q) make -C build test

test: run-tests

benchmark:
	$(Q) util/benchmark.py --graph

metrics:
	$(Q) util/metrics.py

clean: build/Makefile
	$(Q) make -C build clean

obliterate: confirm-obliterate
	$(Q) rm -rf build

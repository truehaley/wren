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

all:
	$(Q) echo "Select what to build with a command similar to 'make what-type' where:"
	$(Q) echo "Options are:   __what__        __type__"
	$(Q) echo "                 cmake           rel"
	$(Q) echo "                 build           dbg"
	$(Q) echo "                 test            f32"
	$(Q) echo "               benchmark        f32dbg"
	$(Q) echo "                 clean           all"
	$(Q) echo "              obliterate         "
	$(Q) exit


######
## cmake commands
.PHONY: cmake-rel cmake-dbg cmake-f32 cmake-f32dbg cmake-all
cmake-rel:
	$(Q) cmake --preset default

cmake-dbg:
	$(Q) cmake --preset dbg

cmake-f32:
	$(Q) cmake --preset f32

cmake-f32dbg:
	$(Q) cmake --preset f32dbg

cmake-all: cmake-rel cmake-dbg cmake-f32 cmake-f32dbg

build/rel/Makefile build/dbg/Makefile build/f32rel/Makefile build/f32dbg/Makefile:
	$(Q) echo "Wren has not been configured, please run 'make cmake' first"
	$(Q) exit 1


######
## build commands
.PHONY: build-rel build-dbg build-f32 build-f32dbg build-all
build-rel: build/rel/Makefile
	$(Q) make -C build/rel wren_test

build-dbg: build/dbg/Makefile
	$(Q) make -C build/dbg wren_test

build-f32: build/f32rel/Makefile
	$(Q) make -C build/f32rel wren_test

build-f32dbg: build/f32dbg/Makefile
	$(Q) make -C build/f32dbg wren_test

build-all: build-dbg build-f32dbg build-rel build-f32

build/rel/wren_test build/dbg/wren_test build/f32rel/wren_test build/f32dbg/wren_test:
	$(Q) echo "Wren has not been built, please run 'make build' first"
	$(Q) exit 1


######
## test commands
.PHONY: test-rel test-dbg test-f32 test-f32dbg test-all
test-rel: build/rel/wren_test
	$(Q) make -C build/rel test

test-dbg: build/dbg/wren_test
	$(Q) make -C build/dbg test

test-f32: build/f32rel/wren_test
	$(Q) make -C build/f32rel test

test-f32dbg: build/f32dbg/wren_test
	$(Q) make -C build/f32dbg test

test-all: test-dbg test-f32dbg test-rel test-f32

# to run an individual test manually, for example:
#  util/test.py core/number --executable build/f32dbg/wren_test --float32


######
## benchmark commands
.PHONY: benchmark-rel benchmark-dbg benchmark-f32 benchmark-f32dbg benchmark-all
benchmark-rel: build/rel/wren_test
	$(Q) util/benchmark.py --graph --executable build/rel/wren_test

benchmark-dbg: build/dbg/wren_test
	$(Q) util/benchmark.py --graph --executable build/dbg/wren_test

benchmark-f32: build/f32rel/wren_test
	$(Q) util/benchmark.py --graph --executable build/f32rel/wren_test

benchmark-f32dbg: build/f32dbg/wren_test
	$(Q) util/benchmark.py --graph --executable build/f32dbg/wren_test

benchmark-all: benchmark-rel benchmark-dbg benchmark-f32 benchmark-f32dbg


######
## metrics commands
.PHONE: metrics
metrics:
	$(Q) util/metrics.py


######
## clean commands
.PHONY: clean-rel clean-dbg clean-f32 clean-f32dbg clean-all
clean-rel: build/rel/Makefile
	$(Q) make -C build/rel clean

clean-dbg: build/dbg/Makefile
	$(Q) make -C build/dbg clean

clean-f32: build/f32rel/Makefile
	$(Q) make -C build/f32rel clean

clean-f32dbg: build/f32dbg/Makefile
	$(Q) make -C build/f32dbg clean

clean-all: clean-rel clean-dbg clean-f32 clean-f32dbg


######
## obliterate commands
.PHONY: confirm-obliterate obliterate-rel obliterate-dbg obliterate-f32 obliterate-f32dbg obliterate-all
confirm-obliterate:
		$(Q) echo "This will remove the entire selected build structure!"
		$(Q) echo "Are you sure? [y/N]" && read ans && [ $${ans:-N} = y ]

obliterate-rel: confirm-obliterate
	$(Q) rm -rf build/rel

obliterate-dbg: confirm-obliterate
	$(Q) rm -rf build/dbg

obliterate-f32: confirm-obliterate
	$(Q) rm -rf build/f32rel

obliterate-f32dbg: confirm-obliterate
	$(Q) rm -rf build/f32dbg

obliterate-all: confirm-obliterate
	$(Q) rm -rf build

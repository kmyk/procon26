.PHONY: all build build/exact build/exact/fast test test/validate

SOLVER ?= exact
CXXFLAGS = -std=c++14 -Wall -DSOLVER=${SOLVER}
SRCS = forward.cpp exact.cpp procon26.cpp common.cpp

all: build

build: build/exact

build/exact:
	${CXX} ${CXXFLAGS} -g -O2 ${SRCS} main.cpp

build/exact/fast:
	${CXX} ${CXXFLAGS} -O3 -DNDEBUG ${SRCS} main.cpp

build/unittest:
	${CXX} ${CXXFLAGS} -O2 -g ${SRCS} unittest.cpp

test:
	make test/unittest
	make test/exact

test/unittest:
	make build/unittest
	./a.out

test/exact:
	make build/exact
	./runtest.sh ./a.out
	# bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.in | ./a.out | ./validator.py -i $$f.in ; done'

test/validate:
	bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.out | ./validator.py -i $$f.in ; done'

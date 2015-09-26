.PHONY: all build build/fast test test/validate

SOLVER ?= exact
CXXFLAGS = -std=c++14 -Wall -DSOLVER=${SOLVER} -DUSE_$(shell echo ${SOLVER} | tr a-z A-Z)
SRCS = forward.cpp exact.cpp procon26.cpp common.cpp
ifdef DEBUG
    CXXFLAGS += -g -DDEBUG -D_GLIBCXX_DEBUG
endif

all: build

build:
	${CXX} ${CXXFLAGS} -g -O2 ${SRCS} main.cpp

build/fast:
	${CXX} ${CXXFLAGS} -O3 -DNDEBUG ${SRCS} main.cpp

build/unittest:
	${CXX} ${CXXFLAGS} -O2 -g ${SRCS} unittest.cpp

test:
	make test/unittest
	make test/solver

test/unittest:
	make build/unittest
	./a.out

test/solver:
	make build
	./runtest.sh ./a.out
	# bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.in | ./a.out | ./validator.py -i $$f.in ; done'

test/validate:
	bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.out | ./validator.py -i $$f.in ; done'

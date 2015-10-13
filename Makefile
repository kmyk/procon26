.PHONY: all build build/fast test test/validate

CXXFLAGS += -std=c++14 -Wall -lboost_system -lboost_filesystem
SRCS = beam_search.cpp procon26.cpp common.cpp
ifdef DEBUG
    CXXFLAGS += -g -DDEBUG -D_GLIBCXX_DEBUG
endif

all: build

build:
	${CXX}  ${SRCS} main.cpp ${CXXFLAGS} -g

build/fast:
	${CXX} ${SRCS} main.cpp ${CXXFLAGS} -O3 -DNDEBUG

build/unittest:
	${CXX} ${SRCS} unittest.cpp ${CXXFLAGS} -O2 -g

test:
	make test/unittest
	# make test/solver

test/unittest:
	make build/unittest
	./a.out

test/solver:
	# make build
	# ./runtest.sh ./a.out
	fasle

test/validate:
	bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.out | ./validator.py -i $$f.in ; done'

.PHONY: all build build/exact build/exact/fast test test/validate

CXXFLAGS = -std=c++14 -Wall

all: build

build: build/exact

build/exact: exact.cpp
	${CXX} ${CXXFLAGS} -g $^

build/exact/fast: exact.cpp
	${CXX} ${CXXFLAGS} -O3 -DNDEBUG $^

test:
	bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.in | ./a.out | ./validator.py -i $$f.in ; done'

test/validate:
	bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.out | ./validator.py -i $$f.in ; done'

.PHONY: all build build/exact build/exact/fast test test/validate

CXXFLAGS = -std=c++14 -Wall

all: build

build: build/exact

build/exact: exact.cpp
	${CXX} ${CXXFLAGS} -g -O2 $^

build/exact/fast: exact.cpp
	${CXX} ${CXXFLAGS} -O3 -DNDEBUG $^

build/unittest: unittest.cpp
	${CXX} ${CXXFLAGS} -O2 -g $^

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

.PHONY: all build build/fast test test/validate

all: build

build:
	${CXX} -std=c++14 -Wall -g bruteforce.cpp

build/fast:
	${CXX} -std=c++14 -Wall -O3 -DNDEBUG bruteforce.cpp

test:
	bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.in | ./a.out | ./validator.py -i $$f.in ; done'

test/validate:
	bash -c 'for f in test/*.in ; do f="$$(echo $$f | sed -e 's/\.in$$//')" ; echo $$f ; cat $$f.out | ./validator.py -i $$f.in ; done'

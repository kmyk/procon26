.PHONY: all build

all: build

build:
	${CXX} -std=c++14 -Iinclude -O3 bruteforce.cpp

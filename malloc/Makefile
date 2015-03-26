#CC=gcc
#CXX=g++
CC=clang
CXX=clang++
#DEBUG=-DCS550_DEBUG -DCS550_DBG_PRINT
DEBUG=

.PHONY: a.out
a.out:
	$(CC) -O $(DEBUG) -c -std=c11 -Wall -Wextra -pedantic malloc.c
	$(CC) -O $(DEBUG) -c -std=c11 -Wall -Wextra -pedantic utility.c
	$(CXX) -O $(DEBUG) -c -std=c++1y -Wall -Wextra -pedantic test.cpp
	$(CXX) -O $(DEBUG) -std=c++1y -Wall -Wextra -pedantic test.o malloc.o utility.o

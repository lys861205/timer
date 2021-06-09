CXX=g++
CX=gcc
CXXFLAGS=-g -std=c++11
CXFLAGS=-g
LDFLAGS=-lpthread #-lmpi


test_timer: Timer.o test_timer.o
	$(CXX) Timer.o test_timer.o -o $@ $(LDFLAGS)

Timer.o:Timer.cc
	$(CXX) -c $<  $(CXXFLAGS)

test_timer.o:test_timer.cc
	$(CXX) -c $<  $(CXXFLAGS)

clean:
	rm -rf timer.o test_timer.o


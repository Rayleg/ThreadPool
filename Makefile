CXX=g++
SRC=threadpool.cpp
CXXFLAGS=-g -Wall

tp: threadpool.cpp
	$(CXX) $(SRC) $(CXXFLAGS) -o tp
test: test_gen.cpp
	$(CXX) $(CXXFLAGS) test_gen.cpp -o testgen


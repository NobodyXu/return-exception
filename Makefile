test: test.cc ret-exception.hpp
	$(CXX) -std=c++17 test.cc -O2 -o $@ && ./$@

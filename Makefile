CXXFLAGS := -O2 -std=c++17
LDFLAGS := -Wl,--strip-all

test: test.cc ret-exception.hpp
	$(CXX) test.cc $(CXXFLAGS) $(LDFLAGS) -o $@
	./$@

clean:
	rm -f test
.PHONY: clean

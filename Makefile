CXXFLAGS := -O2 -std=c++17
LDFLAGS := -Wl,--strip-all

all: test test2 test3

%: %.cc ret-exception.hpp
	$(CXX) test.cc $(CXXFLAGS) $(LDFLAGS) -o $@
	./$@

test2: test2.cc ret-exception.hpp
	$(CXX) test2.cc $(CXXFLAGS) -fno-exceptions $(LDFLAGS) -o $@
	(./$@ && exit 1) || exit 0

clean:
	rm -f test test2 test3
.PHONY: clean all

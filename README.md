# return-exception

Return errors in an unignorable way and have compile-time only Catch, which produces no bloat code with `CXXFLAGS=-O2 -Wl,--strip-all`.

## Usage

```c++
#include "/path/to/ret-exception.hpp"

#include <limits>
#include <stdexcept>

#include <cstdio>
#include <cstdint>
#include <cinttypes>

#include <err.h>

template <class T>
auto ftoi(long double f) noexcept -> Ret_except<T, std::invalid_argument, std::out_of_range>
{
    if (f == std::numeric_limits<float>::infinity())
        return {std::invalid_argument{"f should not be INF"}};
    else if (f > std::numeric_limits<T>::max())
        return {std::out_of_range("f is too large")};
    else if (f < std::numeric_limits<T>::min())
        return {std::out_of_range("f is too small")};
    return static_cast<T>(f);
}

int main(int argc, char* argv[])
{
    long double f;
    std::scanf("%Lf", &f);

    auto integer = ftoi<std::uint64_t>(f)
        .Catch([](const std::out_of_range &e) noexcept {
            // Catch out_of_range only
            errx(1, "std::out_of_range: %s", e.what());
        }).Catch([](const auto &e) noexcept {
            // Catch any exception
            errx(1, "In function %s: Catched exception %s", 
                    __PRETTY_FUNCTION__, e.what());
        }).get_return_value(); /* Get return value here when all exceptions are handled */

    std::printf("%" PRIu64 "\n", integer);
    return 0;
}
```

Now, if you compile the code with `clang++ -std=c++17 -O2 -Wl,--strip-all -fno-exceptions -fno-rtti example.cc`,
you will get an executable that is fairly small (on my x86-64 intel machine, 12K only).

Let's do some test on the code:

```
$ ./a.out
1.2333
1
$ ./a.out
123412434324823878712471230712847312844
a.out: std::out_of_range: f is too large
$ ./a.out
INF
a.out: In function auto main(int, char **)::(anonymous class)::operator()(const auto &) const [e:auto = std::invali
d_argument]: Catched exception f should not be INF
```

It's doing exactly what we want it to do.

What is more amazing is that how small the executable can be.
If you run the compiler again with 
`clang++ -std=c++17 -O3 -flto -Wl,--strip-all -fno-exceptions -fno-rtti example.cc`, you would probably find
the executable to be even smaller.

On my machine, I found it to be merely 8.9K, which definitely is not possible with exceptions on.

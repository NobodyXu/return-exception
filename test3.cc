#include "ret-exception.hpp"
#include <type_traits>
#include <stdexcept>
#include <cassert>

class A {
public:
    A(Ret_except<void, std::exception> &e);
};

using Ret_except_t1 = Ret_except<void, int, void*>;
using Ret_except_t2 = Ret_except<void, void*>;

int main(int argc, char* argv[])
{
    static_assert(std::is_constructible_v<A, Ret_except_detector_t>);

    // cp ctor from other Ret_except
    try {
        Ret_except_t2 r2{static_cast<void*>(nullptr)};
        Ret_except_t1 r1{r2};

        bool is_visited = false;
        r1.Catch([&](void *p) noexcept {
            assert(is_visited == false);
            assert(p == nullptr);
            is_visited = true;
        });
        assert(is_visited);
    } catch (...) {
        assert(false);
    }

    // mv ctor from other Ret_except
    try {
        Ret_except_t1 r1{Ret_except_t2{static_cast<void*>(nullptr)}};

        bool is_visited = false;
        r1.Catch([&](void *p) noexcept {
            assert(is_visited == false);
            assert(p == nullptr);
            is_visited = true;
        });
        assert(is_visited);
    } catch (...) {
        assert(false);
    }

    return 0;
}

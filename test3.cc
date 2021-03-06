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
using Ret_except_t3 = Ret_except<void, int>;

class B {
public:
    using Ret_except_t = Ret_except_t2;
    B(Ret_except_t &e);
};

int main(int argc, char* argv[])
{
    static_assert(std::is_constructible_v<A, Ret_except_detector_t>);

    static_assert(std::is_same_v<glue_ret_except_t<Ret_except_t3, Ret_except_t2>, Ret_except_t1>);
    static_assert(std::is_same_v<glue_ret_except_t<Ret_except<int, int>, Ret_except_t2>, 
                                 Ret_except<int, int, void*>>);

    static_assert(std::is_same_v<glue_ret_except_from_t<A, Ret_except_t2>, Ret_except_t2>);
    static_assert(std::is_same_v<glue_ret_except_from_t<B, Ret_except_t3>, Ret_except_t1>);

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

    // cp ctor of ret from other Ret_except
    try {
        Ret_except<int, void*> r2{-1};
        Ret_except<int, char> r1{r2};

        assert(r2.get_return_value() == -1);
        assert(r1.get_return_value() == -1);
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

    // mv ctor ret from other Ret_except
    try {
        Ret_except<std::vector<int>, void*> r2{std::vector{-1}};
        Ret_except<std::vector<int>, char> r1{std::move(r2)};

        assert(r2.get_return_value().size() == 0);
        assert(r1.get_return_value().size() == 1);
        assert(r1.get_return_value()[0] == -1);
    } catch (...) {
        assert(false);
    }

    return 0;
}

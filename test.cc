#include "ret-exception.hpp"
#include <cassert>

int main(int argc, char* argv[])
{
    try {
        Ret_except<char, int, long, void*> r{std::in_place_type<int>, -1};

        r.Catch([](void *p) {
            assert(false);
        }).Catch([](int i) {
            assert(i == -1);
        }).Catch([](auto &e) {
            assert(false);
        });
    } catch (...) {
        assert(false);
    }

    try {
        Ret_except<char, int, long, void*> r{};

        r.set_exception<long>(-1L);
    } catch (long l) {
        assert(l == -1);
    }

    try {
        Ret_except<char, int> r;

        r.set_return_value('c');
        assert(r.get_return_value() == 'c');
    } catch (...) {
        assert(false);
    }

    try {
        Ret_except<char, int, long, void*> r{-1};
    } catch (int i) {
        assert(i == -1);
    }

    return 0;
}

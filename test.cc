#include "ret-exception.hpp"
#include <cstdio>
#include <cassert>

int main(int argc, char* argv[])
{
    try {
        Ret_except<char, int, long, void*> r{std::in_place_type<int>, -1};

        r.Catch([](void *p) {
            assert(false);
        }).Catch([](int i) {
            std::printf("%u\n", static_cast<unsigned>(i));
        }).Catch([](auto &e) {
            assert(false);
        });

        r.set_return_value<char>('c');
        assert(r.get_return_value() == 'c');

        r.set_exception<long>(-1L);
    } catch (long l) {
        std::printf("%lu\n", static_cast<unsigned long>(l));
    }

    return 0;
}

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
        Ret_except<void, int, long, void*> r{std::in_place_type<int>, -1};

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
        Ret_except<char, int, long, void*> r{std::in_place_type<int>, -1};

        r.Catch([](void *p) {
            assert(false);
        }).Catch([](auto i) {
            // Ensure the function actually get called at runtime takes int
            assert((std::is_same_v<decltype(i), int>));
            if constexpr(std::is_same_v<decltype(i), int>)
                assert(i == -1);
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
        Ret_except<char, int, long, void*> r{-1};
    } catch (int i) {
        assert(i == -1);
    }

    try {
        Ret_except<char, int> r;

        r.set_return_value('c');
        assert(r.get_return_value() == 'c');
    } catch (...) {
        assert(false);
    }

    try {
        Ret_except<char, int> r{'c'};
        assert(r.get_return_value() == 'c');
    } catch (...) {
        assert(false);
    }

    try {
        Ret_except<char, int> r{std::in_place_type<char>, 'c'};
        assert(r.get_return_value() == 'c');
    } catch (...) {
        assert(false);
    }

    try {
        Ret_except<char, int> r{-1};
        assert(r.get_return_value() == 'c');
    } catch (int i) {
        assert(i == -1);
    } catch (...) {
        assert(false);
    }

    return 0;
}

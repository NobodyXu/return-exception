#include "ret-exception.hpp"
#include <type_traits>
#include <stdexcept>

class A {
public:
    A(Ret_except<void, std::exception> &e);
};

int main(int argc, char* argv[])
{
    static_assert(std::is_constructible_v<A, Ret_except_detector_t>);

    return 0;
}

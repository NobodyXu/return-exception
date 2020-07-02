#include "ret-exception.hpp"

int main(int argc, char* argv[]) {
    Ret_except<void, int>{-1};

    return 0;
}

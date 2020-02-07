#include <iostream>
#include "tests.h"

int main()
{
    std::cout << test_memory_management() << '\n';
    std::cout << test_non_movable() << '\n';
    std::cout << test_copy_swap() << '\n';
    std::cout << test_index_loops() << '\n';
    std::cout << test_emplacement() << '\n';
    std::cout << test_weird_alignment() << '\n';
}
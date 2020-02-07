#pragma once

#include <exception>
#include <sstream>
#include <tuple>

#include "custom_vector.h"
#include "test_structs.h"

class test_failed_exception : public std::exception
{
public:
    explicit
        test_failed_exception(const std::string& msg) : error_message(msg) {}

    virtual ~test_failed_exception() throw () {}

    virtual const char* what() const throw ()
    {
        return error_message.c_str();
    }

protected:
    const std::string error_message;
};

template <typename T, typename U>
void require_equal(const std::string& func, const std::string& what, const T& actual, const U& expected)
{
    if (actual != expected)
    {
        std::stringstream ss;
        ss << func << " fails: " << what << " doesn't match expected. Actual: " << actual << ", Expected: " << expected;
        throw test_failed_exception(ss.str());
    }
}

template <typename T, typename U>
void require_unequal(const std::string& func, const std::string& what, const T& actual, const U& expected)
{
    if (actual == expected)
    {
        std::stringstream ss;
        ss << func << " fails: " << what << " should not match. Actual: " << actual << ", Expected: " << expected;
        throw test_failed_exception(ss.str());
    }
}

std::string test_memory_management()
{
    const std::string& func = __FUNCTION__;
    try
    {
        // Helpful little lambdas
        auto check_size = [&func](size_t actual, size_t expected)
        {
            require_equal(func, "vector size", actual, expected);
        };

        auto check_capacity = [&func](size_t actual, size_t expected)
        {
            require_equal(func, "vector capacity", actual, expected);
        };

        auto check_count = [&func](size_t actual, size_t expected)
        {
            require_equal(func, "object count", actual, expected);
        };

        using counter_void_t = counter<void>;

        // Start a scope here so memory gets cleared at the end of it
        {
            custom_vector<counter_void_t> vec;
            counter_void_t cvt;

            // The vector is empty, and we've created 1 counter<void> (which I'll call cvt)
            size_t expected_size = 0;
            size_t expected_capacity = 0;
            size_t expected_count = 1;

            auto check_all = [&]
            {
                check_size(vec.size(), expected_size);
                check_capacity(vec.capacity(), expected_capacity);
                check_count(counter_void_t::total(), expected_count);
            };

            check_all();

            vec.reserve(8);

            // The vector has reserved memory, but the size and amount of cvt have not changed
            expected_capacity = 8;
            check_all();

            vec.push_back(cvt);
            vec.push_back(cvt);
            vec.push_back(cvt);

            // The vector has added 3 more cvt. Both the vector size and amount of cvt should increase by 3
            // No reallocation should take place as the capacity is sufficient
            expected_size += 3;
            expected_count += 3;
            check_all();

            vec.clear();

            // The vector has cleared all memory and destroyed its objects. Reset back to the inital count of objects
            expected_size = 0;
            expected_capacity = 0;
            expected_count = 1;
            check_all();

            vec.push_back(cvt);

            // We've pushed back 1 cvt. Also, we should have reallocated, so update the capacity.
            expected_size += 1;
            expected_capacity += 1;
            expected_count += 1;
            check_all();

            vec.push_back(cvt);
            vec.push_back(cvt);
            vec.push_back(cvt);

            // We've pushed back 3 cvt. Also, we should have reallocated, so update the capacity.
            expected_size += 3;
            expected_capacity += 3;
            expected_count += 3;
            check_all();

            vec.push_back(cvt);

            // We've pushed back 1 cvt. But this time, the capacity should have increased by 2, due to the 1.5x scale factor (4 => 6).
            expected_size += 1;
            expected_capacity += 2;
            expected_count += 1;
            check_all();

            vec.push_back(cvt);

            // We've pushed back 1 cvt. But this time, no reallocation should take place, so no need to update capacity.
            expected_size += 1;
            expected_count += 1;
            check_all();
        }

        check_count(counter_void_t::total(), 0);
    }
    catch (test_failed_exception e)
    {
        return e.what();
    }

    return func + " passed";
}

// won't compile if failed
std::string test_non_movable()
{
    const std::string& func = __FUNCTION__;

    custom_vector<non_movable> vec;
    vec.push_back(non_movable{});

    return func + " passed";
}

std::string test_copy_swap()
{
    const std::string& func = __FUNCTION__;
    try
    {
        // Helpful little lambdas
        auto check_size = [&func](size_t actual, size_t expected)
        {
            require_equal(func, "vector size", actual, expected);
        };

        auto check_elements = [&func](const std::string& actual, const std::string& expected)
        {
            require_equal(func, "vector element", actual, expected);
        };

        auto check_addresses = [&func](std::string* actual, std::string* expected)
        {
            require_unequal(func, "vector address", uintptr_t(actual), uintptr_t(expected));
        };

        custom_vector<std::string> vec1, vec2;

        vec1.push_back("hello ");
        vec1.push_back("world!");

        // Invokes both the copy-constructor and assigment operator
        vec2 = vec1;

        // Both size should be equal here as they were copied
        check_size(vec1.size(), vec2.size());

        // Also the elements should be equal
        check_elements(vec1[0], vec2[0]);
        check_elements(vec1[1], vec2[1]);

        // But not their addresses
        check_addresses(&vec1[0], &vec2[0]);
        check_addresses(&vec1[1], &vec2[1]);

        vec2.clear();

        check_size(vec2.size(), 0);

        // Invokes the move-constructor and assignment operator
        vec2 = std::move(vec1);

        check_size(vec1.size(), 0);
        check_size(vec2.size(), 2);

        check_elements(vec2[0], "hello ");
        check_elements(vec2[1], "world!");
    }
    catch (test_failed_exception e)
    {
        return e.what();
    }

    return func + " passed";
}

std::string test_index_loops()
{
    const std::string& func = __FUNCTION__;
    try
    {
        custom_vector<std::string> vec;

        vec.push_back("hello ");
        vec.push_back("world!");

        std::stringstream ss;

        // using index operator directly
        ss << vec[0] << vec[1];
        require_equal(func, "indexing", ss.str(), "hello world!");

        ss.str("");

        // Using begin and end iterators
        for (auto&& s : vec)
        {
            ss << s;
        }
        require_equal(func, "looping", ss.str(), "hello world!");
    }
    catch (test_failed_exception e)
    {
        return e.what();
    }

    return func + " passed";
}

std::string test_emplacement()
{
    const std::string& func = __FUNCTION__;
    try
    {
        custom_vector<different_variables> vec;

        auto check_element = [&func](auto&& actual, auto&& expected)
        {
            require_equal(func, "emplace element", actual, expected);
        };

        // Constructs objects within the vector instead of outside where they would need copied
        vec.emplace_back(1, 1.5, "hello ");
        vec.emplace_back(2, 2.5, "world!");

        check_element(vec[0].i, 1);
        check_element(vec[0].d, 1.5);
        check_element(vec[0].s, "hello ");
        check_element(vec[1].i, 2);
        check_element(vec[1].d, 2.5);
        check_element(vec[1].s, "world!");
    }
    catch (test_failed_exception e)
    {
        return e.what();
    }

    return func + " passed";
}

std::string test_weird_alignment()
{
    const std::string& func = __FUNCTION__;
    try
    {
        custom_vector<weird_alignment> vec;

        vec.emplace_back();
        vec.emplace_back();
        vec.emplace_back('1', std::initializer_list<int>{1, 2, 3, 4}, '5');

        auto check_element = [&func](auto&& actual, auto&& expected)
        {
            require_equal(func, "emplace element", actual, expected);
        };

        // I want to make sure the weird alignment didn't mess with indexing or access
        check_element(vec[2].c1, '1');
        check_element(vec[2].i[0], 1);
        check_element(vec[2].c2, '5');

        // I pretty much just want this to not crash here
    }
    catch (test_failed_exception e)
    {
        return e.what();
    }

    return func + " passed";
}

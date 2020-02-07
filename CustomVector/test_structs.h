#pragma once

#include <algorithm>
#include <iomanip>
#include <sstream>

template <class T>
class counter
{

public:
    counter() { ++constructs_; }
    counter(const counter& obj) { ++constructs_; }
    counter(counter&& obj) { ++constructs_; }
    ~counter() { ++destructs_; }

    static int total() { return constructs_ - destructs_; }

    static std::string sprint()
    {
        std::stringstream ss;

        ss << "c: " << std::setw(8) << constructs_
            << " d: " << std::setw(8) << destructs_
            << " t: " << std::setw(8) << total();

        return ss.str();
    }

private:
    static size_t constructs_;
    static size_t destructs_;
};

template <typename T>
size_t counter<T>::constructs_ = 0;

template <typename T>
size_t counter<T>::destructs_ = 0;

//------------------------------------------------------------------------------

struct non_copyable
{
    non_copyable() = default;
    non_copyable(const non_copyable& obj) = delete;
    non_copyable& operator=(const non_copyable& obj) = delete;
    non_copyable(non_copyable&& obj) = default;
    non_copyable& operator=(non_copyable&& obj) = default;
};

//------------------------------------------------------------------------------

struct non_movable
{
    non_movable() = default;
    non_movable(const non_movable& obj) = default;
    non_movable& operator=(const non_movable& obj) = default;
    non_movable(non_movable&& obj) = delete;
    non_movable& operator=(non_movable&& obj) = delete;
};

//------------------------------------------------------------------------------

struct different_variables
{
    different_variables() = default;
    different_variables(int new_i, double new_d, const std::string& new_s) :
        i(new_i), d(new_d), s(new_s) {}

    int i;
    double d;
    std::string s;
};

//------------------------------------------------------------------------------

struct weird_alignment
{
    weird_alignment() : c1('0'), c2('1')
    {
        std::fill(std::begin(i), std::end(i), 0);
    }
    weird_alignment(char new_c1, std::initializer_list<int> new_i, char new_c2) : c1(new_c1), c2(new_c2)
    {
        std::copy(std::begin(new_i), std::end(new_i), std::begin(i));
    }

    // a character, and then 32 more bytes, and another character should hopefully mess with the compilers alignment
    char c1;
    uint64_t i[4];
    char c2;
};

#pragma once

template <typename F>
struct defer_body
{
    F f;
    defer_body(F f) : f(f) {}
    ~defer_body() { f(); }
};

template <typename F>
defer_body<F> defer_func(F f) {
    return defer_body<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

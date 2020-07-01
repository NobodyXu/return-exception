#ifndef  __return_exception_HPP__
# define __return_exception_HPP__

# include <variant>
# include <functional>
# include <utility>
# include <type_traits>

namespace {
/**
 * Ret_except forces the exception returned to be handled, otherwise it would be
 * thrown in destructor.
 * 
 * Ts... must not be std::monostate, void or the same type as Ret.
 */
template <class Ret, class ...Ts>
class Ret_except {
    bool is_exception_handled = 0;
    bool has_exception = 0;

    std::conditional_t<std::is_void_v<Ret>, 
                       std::variant<std::monostate, Ts...>,
                       std::variant<std::monostate, Ret, Ts...>> v;

    template <class T>
    static constexpr bool holds_exp_v = (std::is_same_v<T, Ts> || ...);

    void throw_if_hold_exp()
    {
        if (has_exception && !is_exception_handled && !v.valueless_by_exception())
            std::visit([](auto &&e) {
                if (!std::is_same_v<std::decay_t<decltype(e)>, std::monostate>)
                    throw std::move(e);
            }, v);
    }

public:
    Ret_except() = default;

    template <class T, class ...Args, 
              class = std::enable_if_t<(holds_exp_v<T> || std::is_same_v<T, Ret>) && 
                                       std::is_constructible_v<T, Args...>
                                       >
              >
    Ret_except(std::in_place_type_t<T> type, Args &&...args) 
        noexcept(std::is_nothrow_constructible_v<T, Args...>):
            has_exception{!std::is_same_v<T, Ret>}, 
            v{type, std::forward<Args>(args)...}
    {}

    /**
     * cp/mv ctor/assignment overload is not required as C++17 has copy elision guarantee.
     */
    Ret_except(const Ret_except&) = delete;
    Ret_except(Ret_except&&) = delete;

    Ret_except& operator = (const Ret_except&) = delete;
    Ret_except& operator = (Ret_except&&) = delete;

    template <class T, class ...Args, 
              class = std::enable_if_t<holds_exp_v<T> && std::is_constructible_v<T, Args...>>>
    void set_exception(Args &&...args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        is_exception_handled = 0;
        has_exception = 1;
        v.template emplace<T>(std::forward<Args>(args)...);
    }

    template <class T, class ...Args, 
              class = std::enable_if_t<std::is_same_v<T, Ret> && std::is_constructible_v<T, Args...>>>
    void set_return_value(Args &&...args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        has_exception = 1;
        v.template emplace<T>(std::forward<Args>(args)...);
    }

    /**
     * Example:
     *     auto g() -> Ret_except<PageNotFound, std::runtime_error, std::invalid_argument>;
     *     void f()
     *     {
     *         g().Catch([](const std::runtime_error &e) {
     *             return;
     *         }).Catch([](const auto &e) {
     *             throw e;
     *         })
     *     }
     */
    template <class F>
    auto Catch(F &&f) -> Ret_except&
    {
       if (has_exception && !is_exception_handled && !v.valueless_by_exception())
            std::visit([&, this](auto &&e) {
                using Exception_t = std::decay_t<decltype(e)>;

                if (std::is_same_v<Exception_t, std::monostate>)
                    return;

                if constexpr(std::is_invocable_v<std::decay_t<F>, Exception_t>) {
                    is_exception_handled = 1;
                    std::invoke(std::forward<F>(f), std::forward<decltype(e)>(e));
                }
            }, v);

        return *this;
    }

    auto& get_return_value() &
    {
        throw_if_hold_exp();
        return std::get<Ret>(v);
    }
    
    auto& get_return_value() const &
    {
        throw_if_hold_exp();
        return std::get<Ret>(v);
    }

    auto& get_return_value() &&
    {
        throw_if_hold_exp();
        return std::get<Ret>(std::move(v));
    }

    auto& get_return_value() const &&
    {
        throw_if_hold_exp();
        return std::get<Ret>(std::move(v));
    }

    ~Ret_except() noexcept(false)
    {
        throw_if_hold_exp();
    }
};
}

#endif

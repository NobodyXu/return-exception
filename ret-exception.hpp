#ifndef  __return_exception_HPP__
# define __return_exception_HPP__

# include <variant>
# include <functional>
# include <utility>
# include <type_traits>

# if !defined(__EXCEPTIONS) || !defined(__cpp_exceptions)
#  include <err.h>
#  include <cstdio>
# endif

namespace ret_exception::impl {
template <class T>
constexpr auto type_name() -> const char*
{
    return __PRETTY_FUNCTION__ + 49;
}

template <class decay_T, class T>
static constexpr bool is_constructible() noexcept
{
    if constexpr(std::is_pointer_v<decay_T>)
        return true;
    else
        return std::is_constructible_v<decay_T, T>;
}

template <class decay_T, class T>
static constexpr bool is_nothrow_constructible() noexcept
{
    if constexpr(std::is_pointer_v<decay_T>)
        return true;
    else
        return std::is_nothrow_constructible_v<decay_T, T>;
}
} /* namespace ret_exception::impl */

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

    using variant_t = std::conditional_t<std::is_void_v<Ret>, 
                                         std::variant<std::monostate, Ts...>,
                                         std::variant<std::monostate, Ret, Ts...>>;
    variant_t v;

    template <class T>
    static constexpr bool holds_exp_v = (std::is_same_v<T, Ts> || ...);

    template <class T>
    static constexpr bool holds_type_v = std::is_same_v<T, Ret> || holds_exp_v<T>;

    struct Matcher {
        Ret_except &r;
    
        template <class T, class decay_T = std::decay_t<T>,
                  class = std::enable_if_t<holds_exp_v<decay_T> && 
                          ret_exception::impl::is_constructible<decay_T, T>()>>
        void operator () (T &&obj)
            noexcept(ret_exception::impl::is_nothrow_constructible<decay_T, T>())
        {
            r.set_exception<decay_T>(std::forward<T>(obj));
        }
    };

    void throw_if_hold_exp()
    {
        if (has_exception && !is_exception_handled && !v.valueless_by_exception())
            std::visit([this](auto &&e) {
                using Exception_t = std::decay_t<decltype(e)>;
                if constexpr(!std::is_same_v<Exception_t, Ret> && !std::is_same_v<Exception_t, std::monostate>) {
                    is_exception_handled = 1;

# if defined(__EXCEPTIONS) || defined(__cpp_exceptions)
                    throw std::move(e);
# else
                    std::fprintf(stderr, "[Exception%s ", ret_exception::impl::type_name<Exception_t>());

                    if constexpr(std::is_base_of_v<std::exception, Exception_t>)
                        errx(1, "%s", e.what());
                    else if constexpr(std::is_pointer_v<Exception_t>)
                        errx(1, "%p", e);
                    else if constexpr(std::is_integral_v<Exception_t>) {
                        if constexpr(std::is_unsigned_v<Exception_t>)
                            errx(1, "%llu", static_cast<unsigned long long>(e));
                        else
                            errx(1, "%lld", static_cast<long long>(e));
                    } else
                        errx(1, "");
# endif
                }
            }, v);
    }

public:
    Ret_except() = default;

    template <class T, class decay_T = std::decay_t<T>, 
              class = std::enable_if_t<holds_type_v<decay_T> && 
                      ret_exception::impl::is_constructible<decay_T, T>()>>
    Ret_except(T &&obj)
        noexcept(ret_exception::impl::is_nothrow_constructible<decay_T, T>()):
            has_exception{!std::is_same_v<decay_T, Ret>}, 
            v{std::in_place_type<decay_T>, std::forward<T>(obj)}
    {}

    template <class T, class ...Args, 
              class = std::enable_if_t<holds_type_v<T> && std::is_constructible_v<T, Args...>>>
    Ret_except(std::in_place_type_t<T> type, Args &&...args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>):
            has_exception{!std::is_same_v<T, Ret>}, 
            v{type, std::forward<Args>(args)...}
    {}

    /**
     * Suppose r.v currently holds exception E.
     *
     * The 2 ctors below would only cp/mv the variable held by r.v only if
     * E is also in Ts...
     *
     * These ctors won't copy over Ret.
     */
    template <class Ret_t2, class ...Tps>
    Ret_except(Ret_except<Ret_t2, Tps...> &r)
        noexcept(noexcept(r.Catch(Matcher{*this})))
    {
        r.Catch(Matcher{*this});
    }
    template <class Ret_t2, class ...Tps>
    Ret_except(Ret_except<Ret_t2, Tps...> &&r)
        noexcept(noexcept(std::declval<Ret_except<Ret_t2, Tps...>&&>().Catch(Matcher{*this})))
    {
        std::forward<Ret_except<Ret_t2, Tps...>>(r).Catch(Matcher{*this});
    }

    Ret_except(const Ret_except&) = delete;

    /**
     * move constructor is required as NRVO isn't guaranteed to happen.
     */
    Ret_except(Ret_except &&other) noexcept(std::is_nothrow_move_constructible_v<variant_t>):
        is_exception_handled{other.is_exception_handled},
        has_exception{other.has_exception},
        v{std::move(other.v)}
    {
        other.has_exception = 0;
        other.v.template emplace<std::monostate>();
    }

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

    template <class ...Args, 
              class = std::enable_if_t<std::is_constructible_v<Ret, Args...>>>
    void set_return_value(Args &&...args)
        noexcept(std::is_nothrow_constructible_v<Ret, Args...>)
    {
        has_exception = 0;
        v.template emplace<Ret>(std::forward<Args>(args)...);
    }

    /**
     * Example:
     *     auto g() -> Ret_except<void, PageNotFound, std::runtime_error, std::invalid_argument>;
     *     void f()
     *     {
     *         g().Catch([](const std::runtime_error &e) {
     *             return;
     *         }).Catch([](const auto &e) {
     *             throw e;
     *         });
     *     }
     */
    template <class F>
    auto Catch(F &&f) -> Ret_except&
    {
       if (has_exception && !is_exception_handled && !v.valueless_by_exception())
            std::visit([&, this](auto &&e) {
                using Exception_t = std::decay_t<decltype(e)>;

                if constexpr(!std::is_same_v<Exception_t, std::monostate> && !std::is_same_v<Exception_t, Ret>)
                    if constexpr(std::is_invocable_v<std::decay_t<F>, Exception_t>) {
                        is_exception_handled = 1;
                        std::invoke(std::forward<F>(f), std::forward<decltype(e)>(e));
                    }
            }, v);

        return *this;
    }

    /**
     * Example:
     *     auto g() -> Ret_except<std::string, PageNotFound, std::runtime_error, std::invalid_argument>;
     *     void f()
     *     {
     *         auto ret = g();
     *         ret.Catch([](const std::runtime_error &e) {
     *             return;
     *         }).Catch([](const auto &e) {
     *             throw e;
     *         });
     *
     *         auto &s = ret.get_return_value();
     *         std::cout << s << std::endl;
     *     }
     */
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

    auto&& get_return_value() &&
    {
        throw_if_hold_exp();
        return std::get<Ret>(std::move(v));
    }

    auto&& get_return_value() const &&
    {
        throw_if_hold_exp();
        return std::get<Ret>(std::move(v));
    }

    ~Ret_except() noexcept(false)
    {
        throw_if_hold_exp();
    }
};

/**
 * Example usage:
 *
 * Given that:
 *
 *     class A {
 *     public:
 *         A(Ret_except<void, std::exception> &e);
 *     };
 *     class B {
 *     public:
 *     };
 *
 * To tell the different between A (requires Ret_except) and B,
 * use std::is_constructible_v<type, Ret_except_detector_t>.
 */
struct Ret_except_detector_t {
    template <class Ret, class ...Ts>
    operator Ret_except<Ret, Ts...>& () const noexcept;
};

#endif

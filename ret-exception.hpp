#ifndef  __return_exception_HPP__
# define __return_exception_HPP__

# include <functional>
# include <utility>
# include <type_traits>

# if (__cplusplus >= 201703L)
#  include <variant>
# endif

# if !defined(__EXCEPTIONS) || !defined(__cpp_exceptions)
#  include <err.h>
#  include <cstdio>
# endif

template <template <typename...> class variant_t, class Ret, class ...Ts>
class Ret_except_t;

namespace ret_exception::impl {
struct monostate {};

template <class T>
constexpr auto type_name() -> const char*
{
    return __PRETTY_FUNCTION__ + 49;
}

template <template <typename...> class variant>
struct holds_alternative_t {};

# if (__cplusplus >= 201703L)
template <>
struct holds_alternative_t<std::variant> {
    template <class T, class ...Types>
    static constexpr bool holds_alternative_f(const std::variant<Types...> &v) noexcept
    {
        return std::holds_alternative<T>(v);
    }
};
# endif

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

template <class Ret_except_t1, class Ret_except_t2>
class glue_ret_except;

template <template <typename...> class variant1, class Ret1, class ...Ts, 
          template <typename...> class variant2, class Ret2, class ...Tps>
class glue_ret_except<Ret_except_t<variant1, Ret1, Ts...>, Ret_except_t<variant2, Ret2, Tps...>> {
public:
    using type = Ret_except_t<variant1, Ret1, Ts..., Tps...>;
};

// primary template handles types that have no nested ::Ret_except_t member:
template <class T, class Ret_except_t, class = std::void_t<>>
struct glue_ret_except_from {
    using type = Ret_except_t;
};
 
// specialization recognizes types that do have a nested ::Ret_except_t member:
template <class T, class Ret_except_t>
struct glue_ret_except_from<T, Ret_except_t, std::void_t<typename T::Ret_except_t>>:
    public glue_ret_except<Ret_except_t, typename T::Ret_except_t>
{};
} /* namespace ret_exception::impl */

template <class Ret_except_t1, class Ret_except_t2>
using glue_ret_except_t = typename ret_exception::impl::glue_ret_except<Ret_except_t1, Ret_except_t2>::type;

template <class T, class Ret_except_t>
using glue_ret_except_from_t = typename ret_exception::impl::glue_ret_except_from<T, Ret_except_t>::type;

/**
 * Ret_except forces the exception returned to be handled, otherwise it would be
 * thrown in destructor.
 * 
 * @tparam variant must has API identical to std::variant, include ADL-lookupable visit,
 *                 and override ret_exception::impl::holds_alternative_t for variant and
 *                 provides holds_alternative_t<variant>::holds_alternative_f, which should
 *                 has the same API as std::holds_alternative.
 * @tparam Ts... must not be std::monostate, void or the same type as Ret.
 */
template <template <typename...> class variant, class Ret, class ...Ts>
class Ret_except_t {
    using holds_alternative_t = ret_exception::impl::holds_alternative_t<variant>;

    bool is_exception_handled = 0;
    bool has_exception = 0;

    using monostate = ret_exception::impl::monostate;

    using variant_t = std::conditional_t<std::is_void_v<Ret>, 
                                         variant<monostate, Ts...>,
                                         variant<Ret, monostate, Ts...>>;
    variant_t v;

    template <class T>
    static constexpr bool holds_exp_v = (std::is_same_v<T, Ts> || ...);

    template <class T>
    static constexpr bool holds_type_v = std::is_same_v<T, Ret> || holds_exp_v<T>;

    struct Matcher {
        Ret_except_t &r;
    
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
            visit([this](auto &&e) {
                using Exception_t = std::decay_t<decltype(e)>;
                if constexpr(!std::is_same_v<Exception_t, Ret> && !std::is_same_v<Exception_t, monostate>) {
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
    /**
     * If Ret != void, default initializae Ret;
     * Else, construct empty Ret_except that contain no exception.
     */
    Ret_except_t() = default;

    template <class T, class decay_T = std::decay_t<T>, 
              class = std::enable_if_t<holds_type_v<decay_T> && 
                      ret_exception::impl::is_constructible<decay_T, T>()>>
    Ret_except_t(T &&obj)
        noexcept(ret_exception::impl::is_nothrow_constructible<decay_T, T>()):
            has_exception{!std::is_same_v<decay_T, Ret>}, 
            v{std::in_place_type<decay_T>, std::forward<T>(obj)}
    {}

    template <class T, class ...Args, 
              class = std::enable_if_t<holds_type_v<T> && std::is_constructible_v<T, Args...>>>
    Ret_except_t(std::in_place_type_t<T> type, Args &&...args)
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
    template <template <typename...> class variant2, class Ret_t2, class ...Tps>
    Ret_except_t(Ret_except_t<variant2, Ret_t2, Tps...> &r)
        noexcept(noexcept(r.Catch(Matcher{*this}))):
            v{monostate{}}
    {
        r.Catch(Matcher{*this});
    }
    template <template <typename...> class variant2, class Ret_t2, class ...Tps>
    Ret_except_t(Ret_except_t<variant2, Ret_t2, Tps...> &&r)
        noexcept(noexcept(std::declval<Ret_except_t<variant2, Ret_t2, Tps...>&&>().Catch(Matcher{*this}))):
            v{monostate{}}
    {
        std::forward<Ret_except_t<variant2, Ret_t2, Tps...>>(r).Catch(Matcher{*this});
    }

    Ret_except_t(const Ret_except_t&) = delete;

    /**
     * move constructor is required as NRVO isn't guaranteed to happen.
     */
    Ret_except_t(Ret_except_t &&other) 
        noexcept(std::is_nothrow_move_constructible_v<variant_t>):
            is_exception_handled{other.is_exception_handled},
            has_exception{other.has_exception},
            v{std::move(other.v)}
    {
        other.has_exception = 0;
        other.v.template emplace<monostate>();
    }

    Ret_except_t& operator = (const Ret_except_t&) = delete;
    Ret_except_t& operator = (Ret_except_t&&) = delete;

    template <class T, class ...Args, 
              class = std::enable_if_t<holds_exp_v<T> && std::is_constructible_v<T, Args...>>>
    void set_exception(Args &&...args)
    {
        throw_if_hold_exp();

        is_exception_handled = 0;
        has_exception = 1;
        v.template emplace<T>(std::forward<Args>(args)...);
    }

    template <class ...Args, 
              class = std::enable_if_t<std::is_constructible_v<Ret, Args...>>>
    void set_return_value(Args &&...args)
    {
        throw_if_hold_exp();

        has_exception = 0;
        v.template emplace<Ret>(std::forward<Args>(args)...);
    }

    bool has_exception_set() const noexcept
    {
        return has_exception;
    }
    bool has_exception_handled() const noexcept
    {
        return is_exception_handled;
    }

    template <class T>
    bool has_exception_type() const noexcept
    {
        return holds_alternative_t::template holds_alternative_f<T>(v);
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
    auto Catch(F &&f) -> Ret_except_t&
    {
       if (has_exception && !is_exception_handled && !v.valueless_by_exception())
            visit([&, this](auto &&e) {
                using Exception_t = std::decay_t<decltype(e)>;

                if constexpr(!std::is_same_v<Exception_t, monostate> && !std::is_same_v<Exception_t, Ret>)
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
    template <class T = Ret, class = std::enable_if_t<!std::is_void_v<T>>>
    auto& get_return_value() &
    {
        throw_if_hold_exp();
        return std::get<Ret>(v);
    }

    template <class T = Ret, class = std::enable_if_t<!std::is_void_v<T>>>
    auto& get_return_value() const &
    {
        throw_if_hold_exp();
        return std::get<Ret>(v);
    }

    template <class T = Ret, class = std::enable_if_t<!std::is_void_v<T>>>
    auto&& get_return_value() &&
    {
        throw_if_hold_exp();
        return std::get<Ret>(std::move(v));
    }

    template <class T = Ret, class = std::enable_if_t<!std::is_void_v<T>>>
    auto&& get_return_value() const &&
    {
        throw_if_hold_exp();
        return std::get<Ret>(std::move(v));
    }

    template <class T = Ret, class = std::enable_if_t<!std::is_void_v<T>>>
    operator T&() &
    {
        throw_if_hold_exp();
        return std::get<Ret>(v);
    }

    template <class T = Ret, class = std::enable_if_t<!std::is_void_v<T>>>
    operator const T&() const &
    {
        throw_if_hold_exp();
        return std::get<Ret>(v);
    }

    template <class T = Ret, class = std::enable_if_t<!std::is_void_v<T>>>
    operator T&&() &&
    {
        throw_if_hold_exp();
        return std::get<Ret>(std::move(v));
    }

    template <class T = Ret, class = std::enable_if_t<!std::is_void_v<T>>>
    operator const T&&() const &&
    {
        throw_if_hold_exp();
        return std::get<Ret>(std::move(v));
    }

    ~Ret_except_t() noexcept(false)
    {
        throw_if_hold_exp();
    }
};

# if (__cplusplus >= 201703L)
template <class Ret, class ...Ts>
using Ret_except = Ret_except_t<std::variant, Ret, Ts...>;
# endif

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
    template <template <typename...> class variant, class Ret, class ...Ts>
    operator Ret_except_t<variant, Ret, Ts...>& () const noexcept;
};

#endif

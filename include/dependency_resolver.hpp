#pragma once
#ifndef __JASZYK_DEPENDENCY_RESOLVER_HPP__
#define __JASZYK_DEPENDENCY_RESOLVER_HPP__
#include <memory>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <typeindex>
#include <stdexcept>
#include <type_traits>
#include <tuple>
#include <utility>

#ifdef __GNUC__ // Check if using GCC or Clang
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#elif defined(_MSC_VER) // Check if using MSVC
#pragma warning(push)
#pragma warning(disable: 4514)
#endif



namespace jaszyk {
namespace dependency_resolver_impl {
namespace utility {

// loophole
namespace reflections {
    // Based on
// * http://alexpolt.github.io/type-loophole.html
//   https://github.com/alexpolt/luple/blob/master/type-loophole.h
//   by Alexandr Poltavsky, http://alexpolt.github.io
// * https://www.youtube.com/watch?v=UlNUNxLtBI0
//   Better C++14 reflections - Antony Polukhin - Meeting C++ 2018

    // tag<T, N> generates friend declarations and helps with overload resolution.
    // There are two types: one with the auto return type, which is the way we read types later.
    // The second one is used in the detection of instantiations without which we'd get multiple
    // definitions.
    template <typename T, int N>
    struct tag
    {
        friend auto loophole(tag<T, N>);
        constexpr friend int cloophole(tag<T, N>);
    };

    // The definitions of friend functions.
    template <typename T, typename U, int N, bool B,
        typename = typename std::enable_if_t<
        !std::is_same<
        std::remove_cv_t<std::remove_reference_t<T>>,
        std::remove_cv_t<std::remove_reference_t<U>>>::value>>
        struct fn_def
    {
        friend auto loophole(tag<T, N>) { return U{}; }
        constexpr friend int cloophole(tag<T, N>) { return 0; }
    };

    // This specialization is to avoid multiple definition errors.
    template <typename T, typename U, int N>
    struct fn_def<T, U, N, true>
    {
    };

    // This has a templated conversion operator which in turn triggers instantiations.
    // Important point, using sizeof seems to be more reliable. Also default template
    // arguments are "cached" (I think). To fix that I provide a U template parameter to
    // the ins functions which do the detection using constexpr friend functions and SFINAE.
    template <typename T, int N>
    struct c_op
    {
        template <typename U, int M>
        static auto ins(...) -> int;
        template <typename U, int M, int = cloophole(tag<T, M>{}) >
        static auto ins(int) -> char;

        template <typename U, int = sizeof(fn_def<T, U, N, sizeof(ins<U, N>(0)) == sizeof(char)>)>
        operator U();
    };

    // Here we detect the data type field number. The byproduct is instantiations.
    // Uses list initialization. Won't work for types with user-provided constructors.
    // In C++17 there is std::is_aggregate which can be added later.
    template <typename T, int... Ns>
    constexpr int fields_number(...) { return sizeof...(Ns) - 1; }

    template <typename T, int... Ns>
    constexpr auto fields_number(int) -> decltype(T{ c_op<T, Ns>{}... }, 0)
    {
        return fields_number<T, Ns..., sizeof...(Ns)>(0);
    }

    // Here is a version of fields_number to handle user-provided ctor.
    // NOTE: It finds the first ctor having the shortest unambigious set
    //       of parameters.
    template <typename T, int... Ns>
    constexpr auto fields_number_ctor(int) -> decltype(T(c_op<T, Ns>{}...), 0)
    {
        return sizeof...(Ns);
    }

    template <typename T, int... Ns>
    constexpr int fields_number_ctor(...)
    {
        return fields_number_ctor<T, Ns..., sizeof...(Ns)>(0);
    }

    // This is a helper to turn a ctor into a tuple type.
    // Usage is: jaszyk::dependency_resolver_impl::utility::reflections::as_tuple<data_t>
    template <typename T, typename U>
    struct loophole_tuple;

    template <typename T, int... Ns>
    struct loophole_tuple<T, std::integer_sequence<int, Ns...>>
    {
        using type = std::tuple<decltype(loophole(tag<T, Ns>{}))... > ;
    };

    template <typename T>
    using as_tuple =
        typename loophole_tuple<T, std::make_integer_sequence<int, fields_number_ctor<T>(0)>>::type;

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif // !__GNUC__
} // namespace reflections

    /*
        <Exception classes>
    */
    class element_not_found_exception : public std::runtime_error {
    public:
        inline element_not_found_exception() 
            : std::runtime_error("Dependency is not stored in the resolver.") { }
    };

    class missing_scope_exception : public std::runtime_error {
    public:
        inline missing_scope_exception() 
            : std::runtime_error("Usage of scoped dependency without scope.") { }
    };

    /*
        </Exception classes>
    */
    
    /*
        <extensible tuple>

        This class is used to store dependencies in dependency resolver.
        It allows to store dependencies and resolve them later.

        add_singleton<TInterface, TService>(value) - stores singleton value of type TService as TInterface
            * singleton value is resolved once and stored in tuple

        add_transient<TInterface, TService>() - stores transient value of type TService as TInterface
            * transient value is resolved every time it is requested

        add_scoped<TInterface, TService>() - stores scoped value of type TService as TInterface
            * scoped value is resolved once and stored in scope
			* if scope is not provided, missing_scope_exception is thrown

        resolve_object<T>([scope]) - resolves object of type T
			* if object's parameter is not stored in tuple, element_not_found_exception is thrown

    */
    class i_tuple_element;

    class extensible_tuple {
    public:
        extensible_tuple();

        extensible_tuple(const extensible_tuple& other) = delete;

        extensible_tuple(extensible_tuple&& other) noexcept = default;

        extensible_tuple& operator=(const extensible_tuple& other) = delete;

        extensible_tuple& operator=(extensible_tuple&& other) noexcept = default;

        template <typename TInterface, typename TService>
        void add_singleton(const std::shared_ptr<TService>& value);

        template <typename TInterface, typename TService>
        void add_transient();

        template <typename TInterface, typename TService>
        void add_scoped();

        template <typename T>
        std::shared_ptr<T> get() const;

        template <typename T>
        std::shared_ptr<T> resolve_object() const;

        template <typename T, std::size_t... Is>
        std::shared_ptr<T> resolve_object_helper(std::index_sequence<Is...>) const;

        template <typename T>
        std::shared_ptr<T> get(extensible_tuple& scope) const;

        template <typename T>
        std::shared_ptr<T> resolve_object(extensible_tuple& scope) const;

        template <typename T, std::size_t... Is>
        std::shared_ptr<T> resolve_object_helper(extensible_tuple& scope, std::index_sequence<Is...>) const;

        size_t size() const;

        template <typename T>
        auto find() const;

        auto end() const;

    private:
        template <typename T>
        std::shared_ptr<T> get_service() const;

        template <typename T>
        std::shared_ptr<T> get_service(extensible_tuple& scope) const;

        std::vector<std::unique_ptr<i_tuple_element>> elements_;
        std::map<std::type_index, i_tuple_element*> type_index_map_;
    };
    /*==========================*/


    class i_tuple_element {
    public:
        constexpr explicit i_tuple_element(extensible_tuple& my_tuple)
            : my_tuple_(my_tuple) { }

        inline virtual ~i_tuple_element() = default;
    protected:
        extensible_tuple& my_tuple_;
    };



    template <typename T>
    class tuple_element_base : public i_tuple_element {
        using base = i_tuple_element;
    protected:
        using base::my_tuple_;
    public:
        constexpr explicit tuple_element_base(extensible_tuple& my_tuple)
            : base(my_tuple) { }

        inline virtual ~tuple_element_base() = default;
        inline virtual std::shared_ptr<T> value(extensible_tuple& context) = 0;
        inline virtual std::shared_ptr<T> value() = 0;
    };



    template <typename TInterface, typename TService>
    class singleton_tuple_element : public tuple_element_base<TInterface> {
        using base = tuple_element_base<TInterface>;
        using base::my_tuple_;
    public:
        constexpr explicit singleton_tuple_element(extensible_tuple& my_tuple, const std::shared_ptr<TService>& value)
            : base(my_tuple), value_(value) { }

        inline std::shared_ptr<TInterface> value(extensible_tuple&) override {
            return value_;
        }

        inline std::shared_ptr<TInterface> value() override {
            return value_;
        }

    private:
        std::shared_ptr<TInterface> value_;
    };



    template <typename TInterface, typename TService>
    class transient_tuple_element : public tuple_element_base<TInterface> {
        using base = tuple_element_base<TInterface>;
        using base::my_tuple_;
    public:
        constexpr explicit transient_tuple_element(extensible_tuple& my_tuple)
            : base(my_tuple) { }

        inline std::shared_ptr<TInterface> value(extensible_tuple& context) override {
            return my_tuple_.resolve_object<TService>(context);
        }

        inline std::shared_ptr<TInterface> value() override {
            return my_tuple_.resolve_object<TService>();
        }
    };



    template <typename TInterface, typename TService>
    class scoped_tuple_element : public tuple_element_base<TInterface> {
        using base = tuple_element_base<TInterface>;
        using base::my_tuple_;
    public:
        constexpr explicit scoped_tuple_element(extensible_tuple& my_tuple)
            : base(my_tuple) { }

        inline std::shared_ptr<TInterface> value(extensible_tuple& context) override {
            auto it = context.find<TService>();

            if (it == context.end()) {
                context.add_singleton<TService, TService>(my_tuple_.resolve_object<TService>(context));
            }

            return context.get<TService>(context);
        }

        inline std::shared_ptr<TInterface> value() override {
            throw missing_scope_exception();
        }
    };



    inline extensible_tuple::extensible_tuple() {
        elements_.reserve(4);
    }

    template <typename TInterface, typename TService>
    inline void extensible_tuple::add_singleton(const std::shared_ptr<TService>& value) {
        elements_.push_back(std::make_unique<singleton_tuple_element<TInterface, TService>>(*this, value));
        type_index_map_.insert({ typeid(std::shared_ptr<TInterface>), elements_.back().get() });
    }

    template <typename TInterface, typename TService>
    inline void extensible_tuple::add_transient() {
        elements_.push_back(std::make_unique<transient_tuple_element<TInterface, TService>>(*this));
        type_index_map_.insert({ typeid(std::shared_ptr<TInterface>), elements_.back().get() });
    }

    template <typename TInterface, typename TService>
    inline void extensible_tuple::add_scoped() {
        elements_.push_back(std::make_unique<scoped_tuple_element<TInterface, TService>>(*this));
        type_index_map_.insert({ typeid(std::shared_ptr<TInterface>), elements_.back().get() });
    }

    template <typename T>
    inline std::shared_ptr<T> extensible_tuple::get() const {
        return get_service<T>();
    }

    template <typename T>
    inline std::shared_ptr<T> extensible_tuple::resolve_object() const {
        using tuple_type = jaszyk::dependency_resolver_impl::utility::reflections::as_tuple<T>;
        return resolve_object_helper<T>(std::make_index_sequence<std::tuple_size<tuple_type>::value>{});
    }

    template <typename T, std::size_t... Is>
    inline std::shared_ptr<T> extensible_tuple::resolve_object_helper(std::index_sequence<Is...>) const {
        return std::make_shared<T>(get<typename std::tuple_element_t<Is, jaszyk::dependency_resolver_impl::utility::reflections::as_tuple<T>>::element_type>()...);
    }

    template <typename T>
    inline std::shared_ptr<T> extensible_tuple::get(extensible_tuple& scope) const {
        return get_service<T>(scope);
    }

    template <typename T>
    inline std::shared_ptr<T> extensible_tuple::resolve_object(extensible_tuple& scope) const {
        using tuple_type = jaszyk::dependency_resolver_impl::utility::reflections::as_tuple<T>;
        return resolve_object_helper<T>(scope, std::make_index_sequence<std::tuple_size<tuple_type>::value>{});
    }

    template <typename T, std::size_t... Is>
    inline std::shared_ptr<T> extensible_tuple::resolve_object_helper(extensible_tuple& scope, std::index_sequence<Is...>) const {
        return std::make_shared<T>(get<typename std::tuple_element_t<Is, jaszyk::dependency_resolver_impl::utility::reflections::as_tuple<T>>::element_type>(scope)...);
    }

    inline size_t extensible_tuple::size() const {
        return type_index_map_.size();
    }

    template <typename T>
    inline auto extensible_tuple::find() const {
        return type_index_map_.find(typeid(std::shared_ptr<T>));
    }

    inline auto extensible_tuple::end() const {
		return type_index_map_.end();
	}

    template <typename T>
    inline std::shared_ptr<T> extensible_tuple::get_service() const {
        auto it = type_index_map_.find(typeid(std::shared_ptr<T>));

        if (it == type_index_map_.end()) {
            throw element_not_found_exception();
        }

        return static_cast<tuple_element_base<T>*>(it->second)->value();
    }

    template <typename T>
    inline std::shared_ptr<T> extensible_tuple::get_service(extensible_tuple& scope) const {

        auto it = type_index_map_.find(typeid(std::shared_ptr<T>));

        if (it == type_index_map_.end()) {
            throw element_not_found_exception();
        }

        return static_cast<tuple_element_base<T>*>(it->second)->value(scope);
    }

    /*
        </extensible tuple>
    */

} // namespace utility
} // namespace dependency_resolver_impl

    class dependency_resolver {
        using extensible_tuple = ::jaszyk::dependency_resolver_impl::utility::extensible_tuple;
        class scope_type : public extensible_tuple { };
    public:
        using scope = scope_type;

        struct temporary_scope {};

        static scope global_scope;

        using dependency_not_found_exception = ::jaszyk::dependency_resolver_impl::utility::element_not_found_exception;

        using missing_scope_exception = ::jaszyk::dependency_resolver_impl::utility::missing_scope_exception;

        inline dependency_resolver() = default;

        inline dependency_resolver(const dependency_resolver& other) = delete;

        inline dependency_resolver(dependency_resolver&& other) noexcept = default;

        inline dependency_resolver& operator=(const dependency_resolver& other) = delete;

        inline dependency_resolver& operator=(dependency_resolver&& other) noexcept = default;

        template <typename TInterface, typename TService>
        inline void add_singleton(const TService& value) {
            static_assert(!std::is_abstract<TService>::value, "Cannot register abstract type.");
            data_.add_singleton<TInterface, TService>(std::make_shared<TService>(value));
        }

        template <typename TService>
        inline void add_singleton(const TService& value) {
			static_assert(!std::is_abstract<TService>::value, "Cannot register abstract type.");
			data_.add_singleton<TService, TService>(std::make_shared<TService>(value));
		}

        template <typename TService>
        inline void add_singleton() {
            static_assert(!std::is_abstract<TService>::value, "Cannot register abstract type.");
            data_.add_singleton<TService, TService>(data_.resolve_object<TService>());
        }

        template <typename TInterface, typename TService>
        inline void add_singleton() {
            static_assert(!std::is_abstract<TService>::value, "Cannot register abstract type.");
            data_.add_singleton<TInterface, TService>(data_.resolve_object<TService>());
        }

        template <typename TService>
        inline void add_transient() {
            static_assert(!std::is_abstract<TService>::value, "Cannot register abstract type.");
            data_.add_transient<TService, TService>();
        }

        template <typename TInterface, typename TService>
        inline void add_transient() {
            static_assert(!std::is_abstract<TService>::value, "Cannot register abstract type.");
            data_.add_transient<TInterface, TService>();
        }

        template <typename TService>
        inline void add_scoped() {
            static_assert(!std::is_abstract<TService>::value, "Cannot register abstract type.");
            data_.add_scoped<TService, TService>();
        }

        template <typename TInterface, typename TService>
        inline void add_scoped() {
            static_assert(!std::is_abstract<TService>::value, "Cannot register abstract type.");
			data_.add_scoped<TInterface, TService>();
		}

        template <typename T>
        inline std::shared_ptr<T> resolve(scope& scope) const {
            return data_.resolve_object<T>(static_cast<extensible_tuple&>(scope));
        }

        template <typename T>
        inline std::shared_ptr<T> resolve(temporary_scope) const {
            scope_type scope;
            return data_.resolve_object<T>(static_cast<extensible_tuple&>(scope));
        }

        template <typename T>
        inline std::shared_ptr<T> resolve() const {
			return data_.resolve_object<T>();
		}

        inline size_t size() const {
            return data_.size();
        }

        inline scope make_scope() const {
			return scope();
		}

    private:
        extensible_tuple data_;
    };

    dependency_resolver::scope dependency_resolver::global_scope = dependency_resolver::scope();
} // namespace app

namespace cofftea {
    using namespace jaszyk;
}

#endif // !__JASZYK_DEPENDENCY_RESOLVER_HPP__
// ------------------------------------------------------------------------- //
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
// ------------------------------------------------------------------------- //

#ifndef BOOST_MAKE_ARRAY_HPP
#define BOOST_MAKE_ARRAY_HPP

#include <boost/array.hpp>
#include <boost/config.hpp>
#include <boost/type_traits/common_type.hpp>

#ifdef BOOST_NO_VARIADIC_TEMPLATES

// implementation for c++03 without variadic templates
#include <boost/mpl/at.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/preprocessor.hpp>

#ifndef BOOST_MAKE_ARRAY_MAX_ARITY
#define BOOST_MAKE_ARRAY_MAX_ARITY 10
#endif

namespace boost {
namespace detail {

// deduce array type
template<class Args>
struct deduce_type
{
    typedef typename
        boost::mpl::transform<
            Args,
            boost::decay<boost::mpl::_1>
        >::type Args_;

    typedef typename
        boost::mpl::fold<
            Args_,
            typename boost::mpl::at_c<Args_, 0>::type,
            boost::common_type<
                boost::mpl::_1,
                boost::mpl::_2
            >
        >::type type;
};

} // end detail namespace

// make_array with zero-length
template<class T>
inline array<T, 0> make_array()
{
    return array<T, 0>();
}

#define BOOST_MAKE_ARRAY_DETAIL_DECLARE_TEMPLATE_PARAMETERS(z, i, _) \
    BOOST_PP_COMMA_IF(i) BOOST_PP_CAT(class T, i)

#define BOOST_MAKE_ARRAY_DETAIL_LIST_TEMPLATE_PARAMETERS(z, i, _) \
    BOOST_PP_COMMA_IF(i) BOOST_PP_CAT(T, i)

#define BOOST_MAKE_ARRAY_DETAIL_DECLARE_ARGUMENTS(z, i, _) \
    BOOST_PP_COMMA_IF(i) const BOOST_PP_CAT(T, i) BOOST_PP_CAT(&arg, i)

#define BOOST_MAKE_ARRAY_DETAIL_LIST_ARGUMENTS(z, i, _) \
    BOOST_PP_COMMA_IF(i) BOOST_PP_CAT(arg, i)

#define BOOST_MAKE_ARRAY_DETAIL_INSERT_ASSIGN(z, i, _) \
    a[i] = static_cast<T>(BOOST_PP_CAT(arg, i));
  
#define BOOST_MAKE_ARRAY_DETAIL_DEFINE_FUNCTION(n) \
    template< \
        class T, \
        BOOST_PP_REPEAT( \
            n, \
            BOOST_MAKE_ARRAY_DETAIL_DECLARE_TEMPLATE_PARAMETERS, \
            _ \
        ) \
    > \
    inline array<T, n> \
    make_array(BOOST_PP_REPEAT(n, BOOST_MAKE_ARRAY_DETAIL_DECLARE_ARGUMENTS, _)) \
    { \
        array<T, n> a; \
        BOOST_PP_REPEAT(n, BOOST_MAKE_ARRAY_DETAIL_INSERT_ASSIGN, _) \
        return a; \
    } \
    template< \
        BOOST_PP_REPEAT( \
            n, \
            BOOST_MAKE_ARRAY_DETAIL_DECLARE_TEMPLATE_PARAMETERS, \
            _ \
        ) \
    > \
    inline array< \
        typename detail::deduce_type< \
            typename boost::mpl::vector< \
                BOOST_PP_REPEAT( \
                    n, \
                    BOOST_MAKE_ARRAY_DETAIL_LIST_TEMPLATE_PARAMETERS, \
                    _ \
                )>::type \
        >::type, \
        n \
    > \
    make_array(BOOST_PP_REPEAT(n, BOOST_MAKE_ARRAY_DETAIL_DECLARE_ARGUMENTS, _)) \
    { \
        \
        typedef typename \
            detail::deduce_type< \
                typename boost::mpl::vector< \
                    BOOST_PP_REPEAT( \
                        n, \
                        BOOST_MAKE_ARRAY_DETAIL_LIST_TEMPLATE_PARAMETERS, \
                        _ \
                    ) \
                >::type \
            >::type T; \
        return \
            make_array< \
                T, \
                BOOST_PP_REPEAT( \
                    n, \
                    BOOST_MAKE_ARRAY_DETAIL_LIST_TEMPLATE_PARAMETERS, \
                    _ \
                ) \
            >(BOOST_PP_REPEAT(n, BOOST_MAKE_ARRAY_DETAIL_LIST_ARGUMENTS, _)); \
    }

#define BOOST_MAKE_ARRAY_DETAIL_DEFINE_FUNCTIONS(z, i, _) \
    BOOST_MAKE_ARRAY_DETAIL_DEFINE_FUNCTION(BOOST_PP_ADD(i, 1))

// define the make_array() functions
BOOST_PP_REPEAT(BOOST_MAKE_ARRAY_MAX_ARITY,
                BOOST_MAKE_ARRAY_DETAIL_DEFINE_FUNCTIONS,
                _
)

#undef BOOST_MAKE_ARRAY_DETAIL_DECLARE_TEMPLATE_PARAMETERS
#undef BOOST_MAKE_ARRAY_DETAIL_LIST_TEMPLATE_PARAMETERS
#undef BOOST_MAKE_ARRAY_DETAIL_DECLARE_ARGUMENTS
#undef BOOST_MAKE_ARRAY_DETAIL_LIST_ARGUMENTS
#undef BOOST_MAKE_ARRAY_DETAIL_INSERT_ASSIGN
#undef BOOST_MAKE_ARRAY_DETAIL_DEFINE_FUNCTION
#undef BOOST_MAKE_ARRAY_DETAIL_DEFINE_FUNCTIONS

} // end boost namespace

#else

// implementation for c++11 with variadic templates
namespace boost {
namespace detail {

// deduce array type
template<class ExplicitType, class... Args>
struct deduce_type
{
    typedef ExplicitType type;
};

template<class... Args>
struct deduce_type<void, Args...>
{
    typedef typename common_type<Args...>::type type;
};

} // end detail namespace

// make_array with zero-length
template<class T>
inline array<T, 0> make_array()
{
    return array<T, 0>();
}

template<class ExplicitType = void, class... Args>
inline array<
    typename detail::deduce_type<ExplicitType, Args...>::type,
    sizeof...(Args)
>
make_array(Args&&... args)
{
    typedef typename detail::deduce_type<ExplicitType, Args...>::type T;

    return
        array<T, sizeof...(Args)> {{
            static_cast<T>(std::forward<Args>(args))...
        }};
}

} // end boost namespace

#endif // BOOST_NO_VARIADIC_TEMPLATES

#endif // BOOST_MAKE_ARRAY_HPP

// CxxSwizzle
// Copyright (c) 2013, Piotr Gwiazdowski <gwiazdorrr+github at gmail.com>
#pragma once

#include <type_traits>
#include <iostream>

#include <swizzle/detail/utils.h>
#include <swizzle/detail/common_binary_operators.h>
#include <swizzle/detail/vector_base.h>
#include <swizzle/detail/indexed_vector_iterator.h>
#include <swizzle/glsl/detail/vector_functions_adapter.h>
#include <swizzle/glsl/naive/vector_traits_impl.h>
#include <swizzle/glsl/naive/vector_helper.h>
#include <swizzle/glsl/matrix.h>
#include <swizzle/detail/static_functors.h>

namespace swizzle
{
    namespace glsl
    {
        //! A version with Size == 1 is a special case that should not be created explicitly; it only lives in function proxies and
        //! to create a "common" type for different set of parameters. It is implicitly constructible from a scalar and has to-scalar
        //! conversion operator - this combination is a short path to ambiguity errors when combined with operator overloading.
        //! Why std::conditional shenanegance and not template specialisation? Well, the former needs less code in this case.
        template < class ScalarType, size_t Size >
        class vector : 
            // let the helper decide which base class to choose
            public vector_helper<ScalarType, Size>::base_type,
            // add static glsl functions
            public detail::glsl::vector_functions_adapter<
                // enable binary operators; if there's only one component do not do it, because it would
                // cause ambiguity due to conversion to scalar type operator
                typename std::conditional< 
                    Size == 1,
                    detail::nothing,
                    detail::common_binary_operators<vector<ScalarType, Size>, ScalarType>
                >::type,
                vector,
                ScalarType,
                Size
            >
        {
            static_assert(Size >= 1, "Size must be >= 1");

            //! A convenient mnemonic for base type
            typedef typename vector_helper<ScalarType, Size>::base_type base_type;
            //! "Hide" m_data from outside and make it locally visible
            using base_type::m_data;

        // TYPEDEFS
        public:
            //! Get the real, real internal scalar type. Useful when scalar visible
            //! externally is different than the internal one (well, hello SIMD)
            typedef typename std::remove_reference<decltype(detail::declval<base_type>().m_data[0])>::type internal_scalar_type;
            //! A helpful mnemonic
            typedef std::is_same<internal_scalar_type, scalar_type> are_scalar_types_same;

            //! Number of components of this vector.
            static const size_t num_of_components = Size;
            //! This type.
            typedef vector vector_type;
            //! Scalar type.
            typedef ScalarType scalar_type;
            //!
            typedef bool bool_type;

            //! Type static functions return; for single-component they decay to a scalar
            typedef typename std::conditional<Size==1, scalar_type, vector>::type decay_type;
            //! Sanity checks
            static_assert( sizeof(base_type) == sizeof(scalar_type) * Size, "Size of the base class is not equal to size of its components, most likely empty base class optimisation failed");

        // CONSTRUCTION
        public:
            //! Default constructor.
            inline vector()
            {
            }

            //! Copy constructor
            inline vector(const vector& o)
            {
                m_data = o.m_data;
            }

            //! Implicit constructor from scalar-convertible only for one-component vector
            vector(typename std::conditional<Size == 1, const scalar_type&, detail::operation_not_available>::type s)
            {
                detail::static_for2_short<detail::functor_assign>(*this, s);
            }

            //! For vectors bigger than 1 conversion from scalar should be explicit.
            explicit vector( typename std::conditional<Size!=1, const scalar_type&, detail::operation_not_available>::type s )
            {
                detail::static_for2_short<detail::functor_assign>(*this, s);
            }

            // Block of generic proxy-constructos calling construct member function. Compiler
            // will likely optimise this.

            template <class T0, class... T,
                class = typename std::enable_if< 
                    !(Size <= detail::get_total_size<T0, T...>::value - detail::get_total_size<typename detail::last<T0, T...>::type >::value) &&
                        (Size <= detail::get_total_size<T0, T...>::value),
                    void>::type 
                >
            explicit vector(T0&& t0, T&&... ts)
            {
                construct<0>(std::forward<T0>(t0), std::forward<T>(ts)..., detail::nothing{});
            }


        
        // OPERATORS
        public:

            internal_scalar_type& at(size_t i, std::true_type)
            {
                return m_data[i];
            }
            const internal_scalar_type& at(size_t i, std::true_type) const
            {
                return m_data[i];
            }

            scalar_type& at(size_t i, std::false_type)
            {
                static_assert(sizeof(scalar_type) == sizeof(internal_scalar_type), "");
                return *reinterpret_cast<scalar_type*>(&m_data[i]);
            }
            const scalar_type& at(size_t i, std::false_type) const
            {
                static_assert(sizeof(scalar_type) == sizeof(internal_scalar_type), "");
                return *reinterpret_cast<const scalar_type*>(&m_data[i]);
            }

            scalar_type& operator[](size_t i)
            {
                return at(i, are_scalar_types_same());
            }

            const scalar_type& operator[](size_t i) const
            {
                return at(i, are_scalar_types_same());
            }

            template <size_t i>
            inline internal_scalar_type& static_at(std::true_type)
            {
                return m_data[i];
            }

            template <size_t i>
            inline const internal_scalar_type& static_at(std::true_type) const
            {
                return m_data[i];
            }

            template <size_t i>
            inline scalar_type& static_at(std::false_type)
            {
                static_assert(sizeof(scalar_type) == sizeof(internal_scalar_type), "");
                return *reinterpret_cast<scalar_type*>(&m_data[i]);
            }

            template <size_t i>
            inline const scalar_type& static_at(std::false_type) const
            {
                static_assert(sizeof(scalar_type) == sizeof(internal_scalar_type), "");
                return *reinterpret_cast<const scalar_type*>(&m_data[i]);
            }

            template <size_t i>
            inline scalar_type& static_at()
            {
                return static_at<i>(are_scalar_types_same());
            }

            template <size_t i>
            inline const scalar_type& static_at() const
            {
                return static_at<i>(are_scalar_types_same());
            }

            // Assignment-operation with vector argument

            inline vector& operator+=(const vector& o)
            {
                return detail::static_for2_short<detail::functor_add>(*this, o);
            }
            inline vector& operator-=(const vector& o)
            {
                return detail::static_for2_short<detail::functor_sub>(*this, o);
            }
            inline vector& operator*=(const vector& o)
            {
                return detail::static_for2_short<detail::functor_mul>(*this, o);
            }
            inline vector& operator/=(const vector& o)
            {
                return detail::static_for2_short<detail::functor_div>(*this, o);
            }

            // Assignment-operation with scalar argument

            inline vector& operator+=(const scalar_type& o)
            {
                return detail::static_for2_short<detail::functor_add>(*this, o);
            }
            inline vector& operator-=(const scalar_type& o)
            {
                return detail::static_for2_short<detail::functor_sub>(*this, o);
            }
            inline vector& operator*=(const scalar_type& o)
            {
                return detail::static_for2_short<detail::functor_mul>(*this, o);
            }
            inline vector& operator/=(const scalar_type& o)
            {
                return detail::static_for2_short<detail::functor_div>(*this, o);
            }

            // Matrix multiply operation
            vector& operator*=(const matrix<::swizzle::glsl::vector, ScalarType, Size, Size>& m)
            {
                return *this = *this * m;
            }

            // Others

            inline vector& operator=(const vector& o)
            {
                m_data = o.m_data;
                return *this;
            }

            inline bool operator==(const vector& o) const
            {
                bool are_equal = true;
                detail::static_for2_short<detail::functor_equals>(*this, o, are_equal);
                return are_equal;
            }

            inline bool operator!=(const vector& o) const
            {
                return !(*this == o);
            }

            inline vector operator-() const
            {
                vector result;
                detail::static_for2_short<detail::functor_neg>(result, *this);
                return result;
            }

            // Conversion operator

            //! Auto-decay to scalar type only if this is a 1-sized vector
            inline  operator typename std::conditional<Size == 1, scalar_type, detail::operation_not_available>::type() const
            {
                return static_at<0>();
            } 

        // AUXILIARY
        public:
            //! Decays the vector. For Size==1 this is going to return a scalar, for all other sizes - same vector
            inline decay_type decay() const
            {
                return static_cast<const decay_type&>(*this);
            }
            
        private:

            template <size_t offset, class T0, class... Tail>
            void construct(T0&& t0, Tail&&... tail)
            {
                compose<offset>(detail::decay(std::forward<T0>(t0)));
                construct<offset + detail::get_total_size<T0>::value>(std::forward<Tail>(tail)...);
            }

            template <size_t>
            void construct(detail::nothing)
            {}

            //! Puts scalar at given position. Used only during construction.
            template <size_t N>
            void compose(const scalar_type& v)
            {
                static_at<N>() = v;
            }

            //! Puts a vector (or it's part) at given position. Used only during construction.
            template <size_t N, class TOtherScalar, size_t OtherSize>
            void compose( const vector<TOtherScalar, OtherSize>& v )
            {
                const size_t limit = (N + OtherSize > Size) ? Size : (N + OtherSize);
                detail::functor_compose_from_other_vector<vector, N, vector<TOtherScalar, OtherSize> > functor;
                detail::static_for2<N, limit>(functor, *this, v);
            }

            //! Iterates over the vector, firing Func for each index
            template <class Func>
            static void iterate(Func func)
            {
                detail::static_for<0, Size>(func);
            }

        
        // STL COMPABILITY (not needed, but useful for testing)
        public:
            //!
            typedef detail::indexed_vector_iterator<const vector> const_iterator;
            //!
            typedef detail::indexed_vector_iterator<vector> iterator;

            //! \return Immutable iterator.
            const_iterator begin() const
            {
                return detail::make_indexed_vector_iterator(*this);
            }
            //! \return Immutable iterator.
            const_iterator end() const
            {
                return detail::make_indexed_vector_iterator(*this, num_of_components);
            }
            //! \return Mutable iterator.
            iterator begin()
            {
                return detail::make_indexed_vector_iterator(*this);
            }
            //! \return Mutable iterator.
            iterator end()
            {
                return detail::make_indexed_vector_iterator(*this, num_of_components);
            }

            //! As an inline friend function, because thanks to that all convertibles will use same function.
            friend std::ostream& operator<<(std::ostream& os, const vector& vec)
            {
                vec.iterate( [&](size_t i) -> void { os << vec[i] << (i == Size - 1 ? "" : ","); } );
                return os;
            }

            //! As an inline friend function, because thanks to that all convertibles will use same function.
            friend std::istream& operator>>(std::istream& is, vector& vec)
            {
                vec.iterate( [&](size_t i) -> void 
                { 
                    is >> vec[i];
                    if ( i < Size - 1 && is.peek() == ',')
                    {
                        is.ignore(1);
                    }
                });
                return is;
            }
        };
    }
}

// I, Howard Hinnant, hereby place this code in the public domain.

#ifdef _MSC_VER
   #pragma warning (push)
   //Disable "multiple copy constructors specified"
   #pragma warning (disable : 4521)
   #pragma warning (disable : 4522)
#endif

#include <boost/compressed_pair.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <cstddef>

namespace boost_ext{

namespace detail
{

struct two {char _[2];};

namespace pointer_type_imp
{

template <class U> static two  test(...);
template <class U> static char test(typename U::pointer* = 0);

}

template <class T>
struct has_pointer_type
{
    static const bool value = sizeof(pointer_type_imp::test<T>(0)) == 1;
};

namespace pointer_type_imp
{

template <class T, class D, bool = has_pointer_type<D>::value>
struct pointer_type
{
    typedef typename D::pointer type;
};

template <class T, class D>
struct pointer_type<T, D, false>
{
    typedef T* type;
};

}

template <class T, class D>
struct pointer_type
{
    typedef typename pointer_type_imp::pointer_type<T,
        typename boost::remove_reference<D>::type>::type type;
};

}

template <class T>
struct default_delete
{
    default_delete() {}
    template <class U> default_delete(const default_delete<U>&) {}
    void operator() (T* ptr) const
    {
        BOOST_STATIC_ASSERT(sizeof(T) > 0);
        delete ptr;
    }
};

template <class T>
struct default_delete<T[]>
{
    void operator() (T* ptr) const
    {
        BOOST_STATIC_ASSERT(sizeof(T) > 0);
        delete [] ptr;
    }
};

template <class T, std::size_t N>
struct default_delete<T[N]>
{
    void operator() (T* ptr, std::size_t) const
    {
        BOOST_STATIC_ASSERT(sizeof(T) > 0);
        delete [] ptr;
    }
};

template <class T, class D> class unique_ptr;

namespace detail
{

template <class T> struct unique_ptr_error;

template <class T, class D>
struct unique_ptr_error<const unique_ptr<T, D> >
{
    typedef unique_ptr<T, D> type;
};

}  // detail

template <class T, class D = default_delete<T> >
class unique_ptr
{
    struct nat {int for_bool_;};
    typedef typename boost::add_reference<D>::type deleter_reference;
    typedef typename boost::add_reference<const D>::type deleter_const_reference;
public:
    typedef T element_type;
    typedef D deleter_type;
    typedef typename detail::pointer_type<T, D>::type pointer;

    // constructors
    unique_ptr() : ptr_(pointer()) {}
    explicit unique_ptr(pointer p) : ptr_(p) {}
    unique_ptr(pointer p, typename boost::mpl::if_<
                         boost::is_reference<D>,
                         D,
                         typename boost::add_reference<const D>::type>::type d)
        : ptr_(p, d) {}
    unique_ptr(const unique_ptr& u)
        : ptr_(const_cast<unique_ptr&>(u).release(), u.get_deleter()) {}

    template <class U, class E>
        unique_ptr(const unique_ptr<U, E>& u,
            typename boost::enable_if_c<
                boost::is_convertible<typename unique_ptr<U, E>::pointer, pointer>::value &&
                boost::is_convertible<E, D>::value &&
                (
                    !boost::is_reference<D>::value ||
                     boost::is_same<D, E>::value
                )
                ,
                nat
                >::type = nat())
            : ptr_(const_cast<unique_ptr<U,E>&>(u).release(), u.get_deleter()) {}

    // destructor
    ~unique_ptr() {reset();}

    // assignment
    unique_ptr& operator=(const unique_ptr& cu)
   {
        unique_ptr& u = const_cast<unique_ptr&>(cu);
        reset(u.release());
        ptr_.second() = u.get_deleter();
        return *this;
    }
    template <class U, class E>
    unique_ptr& operator=(const unique_ptr<U, E>& cu)
    {
        unique_ptr<U, E>& u = const_cast<unique_ptr<U, E>&>(cu);
        reset(u.release());
        ptr_.second() = u.get_deleter();
        return *this;
    }
    unique_ptr& operator=(int nat::*)
    {
        reset();
        return *this;
    }

    // observers
    typename boost::add_reference<T>::type operator*()  const {return *ptr_.first();}
    pointer operator->() const {return ptr_.first();}
    pointer get()        const {return ptr_.first();}
    deleter_reference       get_deleter()       {return ptr_.second();}
    deleter_const_reference get_deleter() const {return ptr_.second();}
    operator int nat::*() const {return ptr_.first() ? &nat::for_bool_ : 0;}

    // modifiers
    pointer release()
    {
        pointer tmp = ptr_.first();
        ptr_.first() = pointer();
        return tmp;
    }
    void reset(pointer p = pointer())
    {
        if (ptr_.first() != p)
        {
            if (ptr_.first())
                ptr_.second()(ptr_.first());
            ptr_.first() = p;
        }
    }
    void swap(unique_ptr& u) {ptr_.swap(u.ptr_);}
private:
    boost::compressed_pair<pointer, D> ptr_;

    unique_ptr(unique_ptr&);
    template <class U, class E> unique_ptr(unique_ptr<U, E>&);
    template <class U> unique_ptr(U&, typename detail::unique_ptr_error<U>::type = 0);

    
    unique_ptr& operator=(unique_ptr&);
    template <class U, class E> unique_ptr& operator=(unique_ptr<U, E>&);
    template <class U> typename detail::unique_ptr_error<U>::type operator=(U&);
};

template <class T, class D>
class unique_ptr<T[], D>
{
    struct nat {int for_bool_;};
    typedef typename boost::add_reference<D>::type deleter_reference;
    typedef typename boost::add_reference<const D>::type deleter_const_reference;
public:
    typedef T element_type;
    typedef D deleter_type;
    typedef typename detail::pointer_type<T, D>::type pointer;

    // constructors
    unique_ptr() : ptr_(pointer()) {}
    explicit unique_ptr(pointer p) : ptr_(p) {}
    unique_ptr(pointer p, typename boost::mpl::if_<
                          boost::is_reference<D>,
                          D,
                          typename boost::add_reference<const D>::type>::type d)
        : ptr_(p, d) {}
    unique_ptr(const unique_ptr& u)
        : ptr_(const_cast<unique_ptr&>(u).release(), u.get_deleter()) {}

    // destructor
    ~unique_ptr() {reset();}

    // assignment
    unique_ptr& operator=(const unique_ptr& cu)
    {
        unique_ptr& u = const_cast<unique_ptr&>(cu);
        reset(u.release());
        ptr_.second() = u.get_deleter();
        return *this;
    }
    unique_ptr& operator=(int nat::*)
    {
        reset();
        return *this;
    }

    // observers
    typename boost::add_reference<T>::type operator[](std::size_t i)  const {return ptr_.first()[i];}
    pointer get()        const {return ptr_.first();}
    deleter_reference       get_deleter()       {return ptr_.second();}
    deleter_const_reference get_deleter() const {return ptr_.second();}
    operator int nat::*() const {return ptr_.first() ? &nat::for_bool_ : 0;}

    // modifiers
    pointer release()
    {
        pointer tmp = ptr_.first();
        ptr_.first() = pointer();
        return tmp;
    }
    void reset(pointer p = pointer())
    {
        if (ptr_.first() != p)
        {
            if (ptr_.first())
                ptr_.second()(ptr_.first());
            ptr_.first() = p;
        }
    }
    void swap(unique_ptr& u) {ptr_.swap(u.ptr_);}
private:
    boost::compressed_pair<pointer, D> ptr_;

    template <class U, class E> unique_ptr(U p, E,
        typename boost::enable_if<boost::is_convertible<U, pointer> >::type* = 0);
    template <class U> explicit unique_ptr(U,
        typename boost::enable_if<boost::is_convertible<U, pointer> >::type* = 0);

    unique_ptr(unique_ptr&);
    template <class U> unique_ptr(U&, typename detail::unique_ptr_error<U>::type = 0);

    unique_ptr& operator=(unique_ptr&);
    template <class U> typename detail::unique_ptr_error<U>::type operator=(U&);
};

template <class T, class D, std::size_t N>
class unique_ptr<T[N], D>
{
    struct nat {int for_bool_;};
    typedef typename boost::add_reference<D>::type deleter_reference;
    typedef typename boost::add_reference<const D>::type deleter_const_reference;
public:
    typedef T element_type;
    typedef D deleter_type;
    typedef typename detail::pointer_type<T, D>::type pointer;
    static const std::size_t size = N;

    // constructors
    unique_ptr() : ptr_(pointer()) {}
    explicit unique_ptr(pointer p) : ptr_(p) {}
    unique_ptr(pointer p, typename boost::mpl::if_<
                         boost::is_reference<D>,
                         D,
                         typename boost::add_reference<const D>::type>::type d)
        : ptr_(p, d) {}
    unique_ptr(const unique_ptr& u)
        : ptr_(const_cast<unique_ptr&>(u).release(), u.get_deleter()) {}

    // destructor
    ~unique_ptr() {reset();}

    // assignment
    unique_ptr& operator=(const unique_ptr& cu)
    {
        unique_ptr& u = const_cast<unique_ptr&>(cu);
        reset(u.release());
        ptr_.second() = u.get_deleter();
        return *this;
    }
    unique_ptr& operator=(int nat::*)
    {
        reset();
        return *this;
    }

    // observers
    typename boost::add_reference<T>::type operator[](std::size_t i)  const {return ptr_.first()[i];}
    pointer get()        const {return ptr_.first();}
    deleter_reference       get_deleter()       {return ptr_.second();}
    deleter_const_reference get_deleter() const {return ptr_.second();}
    operator int nat::*() const {return ptr_.first() ? &nat::for_bool_ : 0;}

    // modifiers
    pointer release()
    {
        pointer tmp = ptr_.first();
        ptr_.first() = pointer();
        return tmp;
    }
    void reset(pointer p = pointer())
    {
        if (ptr_.first() != p)
        {
            if (ptr_.first())
                ptr_.second()(ptr_.first(), N);
            ptr_.first() = p;
        }
    }
    void swap(unique_ptr& u) {ptr_.swap(u.ptr_);}
private:
    boost::compressed_pair<pointer, D> ptr_;

    template <class U, class E> unique_ptr(U p, E,
        typename boost::enable_if<boost::is_convertible<U, pointer> >::type* = 0);
    template <class U> explicit unique_ptr(U,
        typename boost::enable_if<boost::is_convertible<U, pointer> >::type* = 0);

    unique_ptr(unique_ptr&);
    template <class U> unique_ptr(U&, typename detail::unique_ptr_error<U>::type = 0);

    unique_ptr& operator=(unique_ptr&);
    template <class U> typename detail::unique_ptr_error<U>::type operator=(U&);
};

template <class T, class D>
inline
void swap(unique_ptr<T, D>& x, unique_ptr<T, D>& y) {x.swap(y);}

template <class T1, class D1, class T2, class D2>
inline
bool
operator==(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {return x.get() == y.get();}

template <class T1, class D1, class T2, class D2>
inline
bool
operator!=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {return x.get() != y.get();}

template <class T1, class D1, class T2, class D2>
inline
bool
operator <(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {return x.get() < y.get();}

template <class T1, class D1, class T2, class D2>
inline
bool
operator<=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {return x.get() <= y.get();}

template <class T1, class D1, class T2, class D2>
inline
bool
operator >(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {return x.get() > y.get();}

template <class T1, class D1, class T2, class D2>
inline
bool
operator>=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {return x.get() >= y.get();}

template <class T, class D>
inline
unique_ptr<T, D>
move(unique_ptr<T, D>& p)
{
    return unique_ptr<T, D>(p.release(), p.get_deleter());
}

}  //namespace boost_ext{

#ifdef _MSC_VER
   #pragma warning (pop)
#endif

// I, Howard Hinnant, hereby place this code in the public domain.

namespace std { namespace detail {

template <bool cond, class Then, class Else>
struct select
{
    typedef Then type;
};

template <class Then, class Else>
struct select<false, Then, Else>
{
    typedef Else type;
};

template <bool b, class T = void> struct restrict_to          {};
template <class T>                struct restrict_to<true, T> {typedef T type;};

using boost::compressed_pair;

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
        typename tr1::remove_reference<D>::type>::type type;
};

} }

namespace std
{

template <class T>
struct default_delete
{
    default_delete() {}
    template <class U> default_delete(const default_delete<U>&) {}
    void operator() (T* ptr) const
    {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        delete ptr;
    }
};

template <class T>
struct default_delete<T[]>
{
    void operator() (T* ptr) const
    {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        delete [] ptr;
    }
};

template <class T, std::size_t N>
struct default_delete<T[N]>
{
    void operator() (T* ptr, std::size_t) const
    {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        delete [] ptr;
    }
};

template <class T, class D = default_delete<T> >
class unique_ptr
{
    struct nat {int for_bool_;};
public:
    typedef T element_type;
    typedef D deleter_type;
    typedef typename detail::pointer_type<T, D>::type pointer;

    // constructors
    unique_ptr() : ptr_(pointer()) {}
    explicit unique_ptr(pointer p) : ptr_(p) {}
    unique_ptr(pointer p, typename detail::select<
                                             tr1::is_reference<D>::value,
                                             D,
                                             const D&>::type d)
        : ptr_(p, d) {}
    unique_ptr(pointer p, typename tr1::remove_reference<D>::type&& d)
        : ptr_(p, std::move(d))
    {
        static_assert(!tr1::is_reference<D>::value, "rvalue deleter bound to reference");
    }
    unique_ptr(unique_ptr&& u)
        : ptr_(u.release(), std::forward<D>(u.get_deleter())) {}
    template <class U, class E>
        unique_ptr(unique_ptr<U, E>&& u,
            typename detail::restrict_to<
                tr1::is_convertible<typename unique_ptr<U, E>::pointer, pointer>::value &&
                tr1::is_convertible<E, D>::value &&
                (
                    !tr1::is_reference<D>::value ||
                    tr1::is_same<D, E>::value
                )
                ,
                nat
                >::type = nat())
            : ptr_(u.release(), std::forward<D>(u.get_deleter())) {}

    // destructor
    ~unique_ptr() {reset();}

    // assignment
    unique_ptr& operator=(unique_ptr&& u)
    {
        reset(u.release());
        ptr_.second() = std::move(u.get_deleter());
        return *this;
    }
    template <class U, class E>
    unique_ptr& operator=(unique_ptr<U, E>&& u)
    {
        reset(u.release());
        ptr_.second() = std::move(u.get_deleter());
        return *this;
    }
    unique_ptr& operator=(int nat::*)
    {
        reset();
        return *this;
    }

    // observers
    typename tr1::add_reference<T>::type operator*()  const {return *ptr_.first();}
    pointer operator->() const {return ptr_.first();}
    pointer get()        const {return ptr_.first();}
    D&       get_deleter()       {return ptr_.second();}
    const D& get_deleter() const {return ptr_.second();}
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
    void swap(unique_ptr&& u) {ptr_.swap(u.ptr_);}
private:
    detail::compressed_pair<pointer, D> ptr_;

    unique_ptr(const unique_ptr&);
    template <class U, class E> unique_ptr(const unique_ptr<U, E>&);
    unique_ptr& operator=(const unique_ptr&);
    template <class U, class E> unique_ptr& operator=(const unique_ptr<U, E>&);
};

template <class T, class D>
class unique_ptr<T[], D>
{
    struct nat {int for_bool_;};
public:
    typedef T element_type;
    typedef D deleter_type;
    typedef typename detail::pointer_type<T, D>::type pointer;

    // constructors
    unique_ptr() : ptr_(pointer()) {}
    explicit unique_ptr(pointer p) : ptr_(p) {}
    unique_ptr(pointer p, typename detail::select<
                                             tr1::is_reference<D>::value,
                                             D,
                                             const D&>::type d)
        : ptr_(p, d) {}
    unique_ptr(pointer p, typename tr1::remove_reference<D>::type&& d)
        : ptr_(p, std::move(d))
    {
        static_assert(!tr1::is_reference<D>::value, "rvalue deleter bound to reference");
    }
    unique_ptr(unique_ptr&& u)
        : ptr_(u.release(), std::forward<D>(u.get_deleter())) {}

    // destructor
    ~unique_ptr() {reset();}

    // assignment
    unique_ptr& operator=(unique_ptr&& u)
    {
        reset(u.release());
        ptr_.second() = std::move(u.get_deleter());
        return *this;
    }
    unique_ptr& operator=(int nat::*)
    {
        reset();
        return *this;
    }

    // observers
    typename tr1::add_reference<T>::type operator[](std::size_t i) const {return ptr_.first()[i];}
    pointer  get()         const {return ptr_.first();}
    D&       get_deleter()       {return ptr_.second();}
    const D& get_deleter() const {return ptr_.second();}
    operator int nat::*()  const {return ptr_.first() ? &nat::for_bool_ : 0;}

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
    void swap(unique_ptr&& u) {ptr_.swap(u.ptr_);}
private:
    detail::compressed_pair<pointer, D> ptr_;

    template <class U> unique_ptr(U p,
        typename detail::select<
                                tr1::is_reference<D>::value,
                                D,
                                const D&>::type d,
        typename detail::restrict_to<tr1::is_convertible<U, pointer>::value>::type* = 0);
    template <class U> unique_ptr(U p, typename tr1::remove_reference<D>::type&& d,
        typename detail::restrict_to<tr1::is_convertible<U, pointer>::value>::type* = 0);
    template <class U> explicit unique_ptr(U,
        typename detail::restrict_to<tr1::is_convertible<U, pointer>::value>::type* = 0);
    unique_ptr(const unique_ptr&);
    unique_ptr& operator=(const unique_ptr&);
};

template <class T, class D, std::size_t N>
class unique_ptr<T[N], D>
{
    struct nat {int for_bool_;};
public:
    typedef T element_type;
    typedef D deleter_type;
    typedef typename detail::pointer_type<T, D>::type pointer;
    static const std::size_t size = N;

    // constructors
    unique_ptr() : ptr_(pointer()) {}
    explicit unique_ptr(pointer p) : ptr_(p) {}
    unique_ptr(pointer p, typename detail::select<
                                             tr1::is_reference<D>::value,
                                             D,
                                             const D&>::type d)
        : ptr_(p, d) {}
    unique_ptr(pointer p, typename tr1::remove_reference<D>::type&& d)
        : ptr_(p, std::move(d))
    {
        static_assert(!tr1::is_reference<D>::value, "rvalue deleter bound to reference");
    }
    unique_ptr(unique_ptr&& u)
        : ptr_(u.release(), std::forward<D>(u.get_deleter())) {}

    // destructor
    ~unique_ptr() {reset();}

    // assignment
    unique_ptr& operator=(unique_ptr&& u)
    {
        reset(u.release());
        ptr_.second() = std::move(u.get_deleter());
        return *this;
    }
    unique_ptr& operator=(int nat::*)
    {
        reset();
        return *this;
    }

    // observers
    typename tr1::add_reference<T>::type operator[](std::size_t i)  const {return ptr_.first()[i];}
    pointer get()          const {return ptr_.first();}
    D&       get_deleter()       {return ptr_.second();}
    const D& get_deleter() const {return ptr_.second();}
    operator int nat::*()  const {return ptr_.first() ? &nat::for_bool_ : 0;}

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
    void swap(unique_ptr&& u) {ptr_.swap(u.ptr_);}
private:
    detail::compressed_pair<pointer, D> ptr_;

    template <class U> unique_ptr(U p,
        typename detail::select<
                                tr1::is_reference<D>::value,
                                D,
                                const D&>::type d,
        typename detail::restrict_to<tr1::is_convertible<U, pointer>::value>::type* = 0);
    template <class U> unique_ptr(U p, typename tr1::remove_reference<D>::type&& d,
        typename detail::restrict_to<tr1::is_convertible<U, pointer>::value>::type* = 0);
    template <class U> explicit unique_ptr(U,
        typename detail::restrict_to<tr1::is_convertible<U, pointer>::value>::type* = 0);
    unique_ptr(const unique_ptr&);
    unique_ptr& operator=(const unique_ptr&);
};

template <class T, class D>
inline
void swap(unique_ptr<T, D>& x, unique_ptr<T, D>& y) {x.swap(y);}

template <class T, class D>
inline
void swap(unique_ptr<T, D>&& x, unique_ptr<T, D>& y) {x.swap(y);}

template <class T, class D>
inline
void swap(unique_ptr<T, D>& x, unique_ptr<T, D>&& y) {x.swap(y);}

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

}  // std
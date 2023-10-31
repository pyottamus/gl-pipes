#include <bitset>
template<typename Container, typename _member_ptr>
class _relthis
{
public:
    using own_type = _relthis<Container, _member_ptr>;

    inline static consteval auto Container::* mem() {
        return _member_ptr{}.template operator() < Container > () ;
    }

    inline static constexpr auto relthis(void* container) {
        return &(static_cast<Container*>(container)->*(mem()));
    }


};
#define relative_this(Container, Name) _relthis<Container, decltype([]<typename T>() consteval {return &T::Name;})>


template<size_t N>
struct min_int;

template<size_t N>
    requires(N > 0 && N <= 8)
struct min_int<N> {
    using type = uint8_t;
};

template<size_t N>
    requires(N > 8 && N <= 16)
struct min_int<N> {
    using type = uint16_t;
};

template<size_t N>
    requires(N > 16 && N <= 32)
struct min_int<N> {
    using type = uint32_t;
};

template<size_t N>
    requires(N > 32 && N <= 64)
struct min_int<N> {
    using type = uint64_t;
};



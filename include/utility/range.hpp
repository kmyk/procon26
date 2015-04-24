#pragma once
template <typename T>
class irange {
    public:
        class iterator {
            public:
                iterator(T value) : value(value), step(1) {}
                iterator(T value, T step) : value(value), step(step) {}
                bool operator != (const iterator & other) const {
                    return value != other.value;
                }
                T const & operator * () const {
                    return value;
                }
                iterator & operator ++ () {
                    value += step;
                    return *this;
                }
            private:
                T value;
                T step;
        };
    public:
        irange(T last) : first(0), last(last), step(1) {}
        irange(T first, T last) : first(first), last(last), step(1) {}
        irange(T first, T last, T step) : first(first), last(last), step(step) {}
        iterator begin() const {
            return iterator(first, step);
        }
        iterator end() const {
            if (step == 0) return iterator(last, step);
            if (0 < step and last < first) return iterator(first, step);
            if (step < 0 and first < last) return iterator(first, step);
            return iterator(first + ((last - first + (0 < step ? -1 : 1)) / step + 1) * step, step);
        }
        typedef T value_type;
    private:
        T const first;
        T const last;
        T const step;
};
typedef irange<long long> lrange;
inline lrange range(lrange::value_type last) { return lrange(last); }
inline lrange range(lrange::value_type first, lrange::value_type last) { return lrange(first, last); }
inline lrange range(lrange::value_type first, lrange::value_type last, lrange::value_type step) { return lrange(first, last, step); }
template <typename T> lrange index_of(const T & a) { return range(a.size()); }
inline lrange range(std::istream & input) { lrange::value_type i; input >> i; return range(i); }
inline lrange reverse_range(lrange::value_type last) { return range(last-1,-1,-1); }
inline lrange reverse_range(lrange::value_type first, lrange::value_type last) { return range(last-1,first-1,-1); }
inline lrange inclusive_range(lrange::value_type last) { return range(last+1); }
inline lrange inclusive_range(lrange::value_type first, lrange::value_type last) { return range(first,last+1); }
template <typename T> lrange reverse_index_of(const T & a) { return range(a.size()-1,-1,-1); }

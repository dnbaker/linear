#ifndef _LINEAR_SET_H__
#define _LINEAR_SET_H__
#include <type_traits>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <vector>
#include <ratio>
#include <memory>
#include <cstdlib>

namespace del {
struct free_deleter{
    template <typename T>
    void operator()(T *p) const {
        std::free(const_cast<std::remove_const_t<T>*>(p));
    }
};
}

namespace linear {

template<typename T, typename SizeType, class ResizeRatio>
class set; // forward declaration

template<typename T, typename SizeType=size_t, class ResizeRatio=std::ratio<3,2>>
void swap(set<T, SizeType, ResizeRatio> &lhs, set<T, SizeType, ResizeRatio> &rhs);


template<typename T, typename SizeType=size_t, class ResizeRatio=std::ratio<3,2>>
class set {
    SizeType n_, m_;
    std::unique_ptr<T, del::free_deleter> data_;

    static constexpr const double RESIZE_FACTOR = static_cast<double>(ResizeRatio::num) / static_cast<double>(ResizeRatio::den);

public:
    using size_type  = SizeType;
    using value_type = T;
    using difference_type = std::make_signed_t<size_type>;
    using pointer_type = T *;

    template<typename... FactoryArgs>
    set(size_type n=0, bool init=!std::is_pod_v<T>,
        FactoryArgs &&...args):
            n_{0}, m_{n}, data_{static_cast<T *>(std::malloc(sizeof(T) * n))} {
        if(data_ == nullptr)
            throw std::bad_alloc();
    }
    set(std::initializer_list<T> l): n_(l.size()), m_(n_), data_{static_cast<T *>(std::malloc(sizeof(T) * n_))} {
        if(data_ == nullptr)
            throw std::bad_alloc();
        std::copy(l.begin(), l.end(), data_);
    }
    set(set &&other): n_(0), m_(0), data_(nullptr) {
        this->swap(other);
    }
    set(const set &other): n_(other.n_), m_(other.m_), data_(static_cast<pointer_type>(std::malloc(sizeof(T) * m_))) {
        if(data_ == nullptr)
            throw std::bad_alloc();
        std::copy(other.begin(), other.end(), begin());
    }
    const T *cbegin() const  {return data_.get();}
    const T *cend()   const  {return data_.get() + n_;}
    const T *begin()  const  {return data_.get();}
    const T *end()    const  {return data_.get() + n_;}
    T *begin()               {return data_.get();}
    T *end()                 {return data_.get() + n_;}
    T *data()                {return data_.get();}
    const T *data()    const {return data_;}
    T &back()                {return data_.get()[n_ - 1];}
    const T &back()    const {return data_.get()[n_ - 1];}
    T &front() {return data_.get()[0];}
    const T &front()   const {return data_.get()[0];}
    auto size()        const {return n_;}
    auto capacity()    const {return m_;}
    T *find(const T &val) {
        return std::find(begin(), end(), val);
    }
    const T *find(const T &val) const {
        return std::find(begin(), end(), val);
    }
    template<typename Predicate>
    auto find_if(const T &val, Predicate p) const {
        return std::find_if(begin(), end(), p);
    }
    bool contains(const T &val) const {
        return find(val) != end();
    }
    T *insert(const T &val) {
        T *it;
        return (it = find(val)) == end() ? &push_back(val): it;
    }
    template<class It1, class It2>
    void insert(It1 it1, It2 it2) {
        while(it1 < it2) insert(*it1++);
    }
    void swap(set &rhs) {
        std::swap(n_, rhs.n_);
        std::swap(m_, rhs.m_);
        std::swap(data_, rhs.data_);
    }
    // Note: push_back and emplace_back are the same *except* that push_back changes size multiplicatively.
    template<typename... Args>
    T &emplace_back(Args&& ... args) {
        if(n_ + 1 > m_)
            reserve(std::max(static_cast<size_type>(m_ * RESIZE_FACTOR), m_ + 1));
        new(data_ + n_++) T(std::forward<Args>(args)...);
        return back();
    }
    template<typename... Args>
    T &emplace_front(Args&& ... args) {
        if(n_ + 1 > m_)
            reserve(std::max(static_cast<size_type>(m_ * RESIZE_FACTOR), m_ + 1));
        std::memmove(data_.get() + sizeof(T), data_, n_ * sizeof(T));
        new(data_) T(std::forward<Args>(args)...);
        ++n_;
        return front();
    }
    T &push_back(const T &val) {
        if(n_ + 1 > m_)
            reserve(std::max(static_cast<size_type>(m_ * RESIZE_FACTOR), m_ + 1));
        data_.get()[n_++] = val;
        return back();
    }
    
    void zero()  {std::memset(data_.get(), 0, sizeof(T) * n_);} // DOES NOT CALL DESTRUCTORS
    void clear() {n_ = 0;}
    bool empty() const {return size() == 0;}
    template<typename Func>
    void for_each(const Func &func) {
        for(T *d = data_.get(), *e = data_.get() + n_; d < e; func(*d++));
    }
    template<typename Func>
    void for_each(const Func &func) const {
        for(const T *d = data_.get(), *e = data_.get() + n_; d < e; func(*d++));
    }
    void reserve(size_t newsize) {
        if(newsize > m_) {
            auto tmp(static_cast<T*>(std::realloc(data_.get(), sizeof(T) * newsize)));
            if(tmp == nullptr) throw std::bad_alloc();
            data_.release();
            data_.reset(tmp);
            m_ = newsize;
        }
    }
    template<typename... Args>
    void resize(size_t newsize, bool init=true, Args &&...args) {
        reserve(newsize);
        if(init) {
            for(;n_ < m_;) new(data_.get() + n_++) T(std::forward<Args>(args)...);
        }
    }
    void shrink_to_fit() {
        if(m_ > n_) {
            auto tmp(static_cast<T*>(std::realloc(data_, sizeof(T) * n_)));
            if(tmp == nullptr) throw std::bad_alloc();
            data_.release();
            data_.reset(tmp);
            m_ = n_;
        }
    }
    T &operator[](size_type idx) {return data_[idx];}
    const T &operator[](size_type idx) const {return data_[idx];}
    ~set(){
        if constexpr(!std::is_trivially_destructible_v<T>) {
            for(auto &el: *this) el.~T();
        }
    }
};
template<typename T, typename SizeType=size_t, class ResizeRatio=std::ratio<3,2>>
void swap(set<T, SizeType, ResizeRatio> &lhs, set<T, SizeType, ResizeRatio> &rhs) {
    lhs.swap(rhs);
}

template<typename K, typename SizeType=std::uint32_t>
class counter {
    // Simple class for a linear-search counting dictionary.
    // This outperforms trees and hash tables for small numbers of element (up to ~100 integers).
    // This is ideal for classifying high-throughput sequencing data, where we know the number of taxids is bounded by the length of the reads.
    std::vector<K> keys_;
    std::vector<SizeType> vals_;
public:
    using size_type = SizeType;
    class iterator {
        counter &ref_;
        unsigned ind_;
    public:
        iterator(counter &ref, size_type ind): ref_(ref), ind_(ind) {}
        bool operator==(const iterator &other) {return ind_ == other.ind_;}
        bool operator<=(const iterator &other) {return ind_ <= other.ind_;}
        bool operator<(const iterator &other) {return ind_ < other.ind_;}
        bool operator>=(const iterator &other) {return ind_ >= other.ind_;}
        bool operator>(const iterator &other) {return ind_ > other.ind_;}
        iterator &operator++() {
            ++ind_; return *this;
        }
        iterator operator++(int) const {
            return iterator(ref_, ind_ + 1);
        }
        auto operator*() const {
            return std::make_pair(std::reference_wrapper(keys_[ind_]), std::reference_wrapper(vals_[ind_]));
        }
    };
    iterator begin() {return iterator(*this, 0);}
    iterator end() {return iterator(*this, keys_.size());}
    size_type add(K &&key, size_type inc=1) {
        if(auto it(std::find(keys_.begin(), keys_.end(), key)); it == keys_.end()) {
            vals_.push_back(inc);
            keys_.push_back(std::move(key));
            return keys_.size() - 1;
        } else {
            const size_type ret = it - keys_.begin();
            vals_[ret] += inc;
            return ret;
        }
    }
    size_type add(const K &key, size_type inc=1) {
        if(auto it(std::find(keys_.begin(), keys_.end(), key)); it == keys_.end()) {
            vals_.push_back(inc);
            keys_.push_back(key);
            return keys_.size() - 1;
        } else {
            const size_type ret(it - keys_.begin());
            vals_[ret] += inc;
            return ret;
        }
    }
    size_type count(const K &key) const {
        if(auto it(std::find(keys_.begin(), keys_.end(), key)); it != keys_.end())
            return vals_[it - keys_.begin()];
        return 0;
    }
    template<typename Q=K, typename T=std::enable_if_t<std::is_arithmetic_v<Q>>>
    void write(std::FILE *fp) {
        std::fprintf(fp, "Size: %zu", size());
        for(size_t i(0); i < size(); ++i) {
            std::fprintf(stderr, "%zu,%zu\n", size_t(keys_[i]), size_t(vals_[i]));
        }
    }
    size_type size() const { return keys_.size();}
    const std::vector<K>        &keys() const {return keys_;}
    const std::vector<SizeType> &vals() const {return vals_;}
};
template<typename K, typename V, typename SizeType=::std::size_t>
class map {
#pragma message("Warning: map is not complete yet.")
    // Simple class for a linear-search counting dictionary.
    // This outperforms trees and hash tables for small numbers of element (up to ~100 integers).
    // This is ideal for classifying high-throughput sequencing data, where we know the number of taxids is bounded by the length of the reads.
    std::vector<K> keys_;
    std::vector<V> vals_;
public:
    using size_type = SizeType;
    map(size_t reserve_size=0) {
        if(reserve_size) {
            keys_.reserve(reserve_size);
            vals_.reserve(reserve_size);
        }
    }
    template <typename K1, typename F, typename... Args>
        std::pair<size_type, bool> uprase_fn(K1 &&key, F fn, Args &&... val) {
        if(auto it(std::find(keys_.begin(), keys_.end(), key)); it == keys_.end()) {
            keys_.emplace_back(key);
            vals_.emplace_back(std::forward<Args>(val)...);
            return std::make_pair(size_type(keys_.size()), false);
        } else {
            const size_type dist = it - keys_.begin();
            fn(keys_[dist], vals_[dist]);
            return std::make_pair(dist, true);
        }
    }
    template<typename Q=V, typename=std::enable_if_t<std::is_trivially_constructible_v<Q>>>
    V &operator[](const K &key) {
        size_type ind;
        if(auto it = std::find(keys_.begin(), keys_.end(), key) ; it == keys_.end()) {
            ind = vals_.size();
            keys_.push_back(key);
            vals_.emplace_back();
        } else ind = it - keys_.begin();
        return vals_[ind];
    }
    V &operator[](const K &key) const {
        size_type ind;
        if(auto it = std::find(keys_.begin(), keys_.end(), key) ; unlikely(it == keys_.end())) {
            throw std::out_of_range(std::string("Key ") + key + " not found");
        } else ind = it - keys_.begin();
        return vals_[ind];
    }
    template<typename Func>
    void for_each(const Func &func) {
        for(size_type i = 0; i < keys_.size(); func(keys_[i], vals_[i]), ++i);
    }
    template<typename Func>
    void for_each(const Func &func) const {
        for(size_type i = 0; i < keys_.size(); func(keys_[i], vals_[i]), ++i);
    }
    size_type size() const {return keys_.size();}
    const std::vector<K>        &keys() const {return keys_;}
    const std::vector<SizeType> &vals() const {return vals_;}
};

} // namespace linear

#endif // #ifndef _LINEAR_SET_H__

#ifndef bqtRangeMapHH
#define bqtRangeMapHH

#include <map>

template<typename Key, typename Value>
class rangemap
{
    struct range
    {
        Key lower, upper;
        
        bool operator< (const range& b) const
        { return lower!=b.lower?lower<b.lower
                               :upper<b.upper; }
        bool operator==(const range& b) const
        { return lower==b.lower&&upper==b.upper; }
        
        bool coincides(const range& b) const
        {
            return lower < b.upper && upper > b.lower;
        }
        bool contains(const Key& v) const { return lower <= v && upper > v; }
    };
    
    typedef std::map<range, Value> Cont;
    Cont data;
public:
    typedef typename Cont::const_iterator const_iterator;
    typedef typename Cont::iterator iterator;

    void clear() { data.clear(); }
    
    void erase(const Key& lo, const Key& up);
    void set(const Key& lo, const Key& up, const Value& v);
    const_iterator find(const Key& lo) const;
    
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }
    
    template<typename Listtype>
    void find_all_coinciding(const Key& lo, const Key& up,
                             Listtype& target);
    
    void compact();
    
    rangemap() {}
    
private:
    rangemap(const rangemap& b);
    void operator=(const rangemap& b);
};

#include "rangemap.tcc"

#endif

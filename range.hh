#ifndef bqt_RangeHH
#define bqt_RangeHH

template<typename Key>
struct rangetype
{
    Key lower, upper;
    
    bool operator< (const rangetype& b) const
    { return lower!=b.lower?lower<b.lower
                           :upper<b.upper; }
    bool operator==(const rangetype& b) const
    { return lower==b.lower&&upper==b.upper; }
    
    bool coincides(const rangetype& b) const
    {
        return lower < b.upper && upper > b.lower;
    }
    bool contains(const Key& v) const { return lower <= v && upper > v; }

    unsigned length() const { return upper - lower; }
};

#endif

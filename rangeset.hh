#ifndef bqtRangeSetHH
#define bqtRangeSetHH

#include <set>
#include "range.hh"

template<typename Key>
class rangeset
{
    typedef rangetype<Key> range;
    typedef std::set<rangetype<Key> > Cont;
    Cont data;
public:
    typedef typename Cont::const_iterator const_iterator;
    typedef typename Cont::iterator iterator;

    void clear() { data.clear(); }
    
    void erase(const Key& lo, const Key& up);
    void set(const Key& lo, const Key& up);
    const_iterator find(const Key& lo) const;
    
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }
    unsigned size() const { return data.size(); }
    
    template<typename Listtype>
    void find_all_coinciding(const Key& lo, const Key& up,
                             Listtype& target);
    
    void compact();
    
    rangeset() {}
    
    // default copy cons. and assign-op. are fine
};

#include "rangeset.tcc"

#endif

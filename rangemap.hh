#ifndef bqtRangeMapHH
#define bqtRangeMapHH

#include "range.hh"

/***************
 *
 * The idea of a rangemap is to have a map<> that allows to
 * have large consequent blocks of repetitive content.
 *
 * Implemented using changepoints.
 *
 * i.e., for a rangemap that has 0..5=A, 6..9=B, 12..15=C
 * you would have a map with: 0=A, 6=B, 10=nil, 12=C, 16=nil
 */
template<typename Key, typename Value>
class rangemap
{
    class Valueholder
    {
        Value val;
        bool nil;
    public:
        Valueholder(): val(), nil(true) {}
        Valueholder(const Value& v): val(v), nil(false) {}
        void set(const Value& v) { val=v; nil=false; }
        void clear() { val=Value(); nil=true; }
        bool is_nil() const { return nil; }
        bool operator==(const Valueholder& b) const { return nil==b.nil && val==b.val; }
        bool operator!=(const Valueholder& b) const { return nil!=b.nil || val!=b.val; }
        const Value& get_value() const { return val; }
    };
    typedef rangecollection<Key, Valueholder> Cont;
    Cont data;
    
public:
    struct const_iterator
    {
        const const_iterator* operator-> () const { return this; }
        typename Cont::const_iterator i;
        rangetype<Key> range;
        Value value;
    public:
        const_iterator(const Cont& c): data(c) { }
        
        bool operator==(const const_iterator& b) const { return i == b.i; }
        bool operator!=(const const_iterator& b) const { return i != b.i; }
        void operator++ ();
        void operator-- ();
        
    private:
        const Cont& data;
        void Reconstruct();
        friend class rangemap;
    };
private:
    const const_iterator ConstructIterator(typename Cont::const_iterator i) const;
    
public:
    rangemap() : data() {}
    
    /* Erase everything between the given range */
    void erase(const Key& lo, const Key& up) { data.erase(lo, up); }
    
    /* Erase a single value */
    void erase(const Key& lo) { data.erase(lo, lo+1); }
    
    /* Modify the given range to have the given value */
    void set(const Key& lo, const Key& up, const Value& v) { data.set(lo, up, v); }
    
    void set(const Key& pos, const Value& v) { set(pos, pos+1, v); }
    
    /* Returns a reference to the given position */
    Value& operator[] (const Key& pos);
    
    /* Find the range that has this value */
    const_iterator find(const Key& v) const { return ConstructIterator(data.find(v)); }
    
    /* Standard functions */
    const_iterator begin() const { return ConstructIterator(data.begin()); }
    const_iterator end() const { return ConstructIterator(data.end()); }
    const_iterator lower_bound(const Key& v) const { return ConstructIterator(data.lower_bound(v)); }
    const_iterator upper_bound(const Key& v) const { return ConstructIterator(data.upper_bound(v)); }
    unsigned size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    void clear() { data.clear(); }
    
    // default copy cons. and assign-op. are fine
};

#include "rangemap.tcc"

#endif

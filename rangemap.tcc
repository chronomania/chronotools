
#include <list>
#include <utility>

template<typename Key, typename Value>
void rangemap<Key,Value>::erase(const Key& lo, const Key& up)
{
    range newrange;
    newrange.lower = lo;
    newrange.upper = up;
    
    typedef std::pair<range, Value> tmpelem;
    typedef std::list<tmpelem> tmplist;
    tmplist newitems;
    
    for(iterator b,a=data.begin(); a!=data.end(); a=b)
    {
        b = a; ++b;
        if(a->first.coincides(newrange))
        {
            if(a->first.lower < lo)
            {
                range lowrange;
                lowrange.lower = a->first.lower;
                lowrange.upper = lo;
                newitems.push_front(std::make_pair(lowrange, a->second));
            }
            if(a->first.upper > up)
            {
                range uprange;
                uprange.lower = up;
                uprange.upper = a->first.upper;
                newitems.push_front(std::make_pair(uprange, a->second));
            }
            data.erase(a);
        }
    }
    for(typename tmplist::const_iterator
            i = newitems.begin();
            i != newitems.end();
            ++i)
    {
        data.insert(*i);
    }
}

template<typename Key, typename Value>
void rangemap<Key,Value>::set(const Key& lo, const Key& up, const Value& v)
{
    erase(lo, up);
    
    range newrange;
    newrange.lower = lo;
    newrange.upper = up;
    data.insert(std::make_pair(newrange, v));
}

template<typename Key, typename Value>
rangemap<Key,Value>::const_iterator rangemap<Key,Value>::find(const Key& v) const
{
    for(const_iterator a=data.begin(); a!=data.end(); ++a)
        if(a->first.contains(v)) return a;
    return data.end();
}

template<typename Key, typename Value>
template<typename Listtype>
void rangemap<Key,Value>::find_all_coinciding
   (const Key& lo, const Key& up,
    Listtype& target)
{
    range newrange;
    newrange.lower = lo;
    newrange.upper = up;
    
    target.clear();
    
    for(const_iterator a=data.begin(); a!=data.end(); ++a)
        if(a->first.coincides(newrange))
            target.push_back(a);
}

template<typename Key, typename Value>
void rangemap<Key,Value>::compact()
{
Retry:
    for(iterator b,a=data.begin(); a!=data.end(); a=b)
    {
        b=a; ++b;
        
        // If the next one is followup to this one
        if(b->first.lower == a->first.upper
        && b->second      == a->second)
        {
            range newrange;
            newrange.lower = a->first.lower;
            newrange.upper = b->first.upper;
            
            data.insert(std::make_pair(newrange, a->second));
            data.erase(a);
            data.erase(b);
            goto Retry;
        }
    }
}


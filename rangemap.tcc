#include "rangemap.hh"


template<typename Key, typename Value>
const typename rangemap<Key,Value>::const_iterator
    rangemap<Key,Value>::ConstructIterator(typename Cont::const_iterator i) const
{
    const_iterator tmp(data);
    while(i != data.end() && i->second.is_nil()) ++i;
    tmp.i = i;
    tmp.Reconstruct();
    return tmp;
}
template<typename Key, typename Value>
void rangemap<Key,Value>::const_iterator::Reconstruct()
{
    if(i != data.end())
    {
        range.lower = i->first;
        typename Cont::const_iterator j = i;
        if(++j != data.end())
            range.upper = j->first;
        else
            range.upper = range.lower;
        
        if(i->second.is_nil())
        {
            fprintf(stderr, "rangemap: internal error\n");
        }
        value = i->second.get_value();
    }
}
template<typename Key, typename Value>
void rangemap<Key,Value>::const_iterator::operator++ ()
{
    /* The last node before end() is always nil. */
    while(i != data.end())
    {
        ++i;
        if(!i->second.is_nil())break;
    }
    Reconstruct();
}
template<typename Key, typename Value>
void rangemap<Key,Value>::const_iterator::operator-- ()
{
    /* The first node can not be nil. */
    while(i != data.begin())
    {
        --i;
        if(!i->second.is_nil())break;
    }
    Reconstruct();
}

#include "rangeset.hh"

template<typename Key>
const typename rangeset<Key>::const_iterator
    rangeset<Key>::ConstructIterator(typename Cont::const_iterator i) const
{
    const_iterator tmp(data);
    while(i != data.end() && i->second.is_nil()) ++i;
    tmp.i = i;
    tmp.Reconstruct();
    return tmp;
}
template<typename Key>
void rangeset<Key>::const_iterator::Reconstruct()
{
    if(i != data.end())
    {
        lower = i->first;
        typename Cont::const_iterator j = i;
        if(++j != data.end())
            upper = j->first;
        else
            upper = lower;
        
        if(i->second.is_nil())
        {
            fprintf(stderr, "rangeset: internal error\n");
        }
    }
}
template<typename Key>
void rangeset<Key>::const_iterator::operator++ ()
{
    /* The last node before end() is always nil. */
    while(i != data.end())
    {
        ++i;
        if(!i->second.is_nil())break;
    }
    Reconstruct();
}
template<typename Key>
void rangeset<Key>::const_iterator::operator-- ()
{
    /* The first node can not be nil. */
    while(i != data.begin())
    {
        --i;
        if(!i->second.is_nil())break;
    }
    Reconstruct();
}
    

#include "miscfun.hh"

template<typename CharT>
const basic_string<CharT>
str_replace(const basic_string<CharT>& search,
            CharT with,
            const basic_string<CharT>& where)
{
    basic_string<CharT> result;
    for(unsigned a=0; a < where.size(); )
    {
        unsigned b = where.find(search, a);
        if(b == where.npos)
        {
            //result += where.substr(a);
            result.insert(result.end(), where.begin()+a, where.end());
            break;
        }
        //result += where.substr(a, b-a);
        result.insert(result.end(), where.begin()+a, where.begin()+b);
        result += with;
        a = b + search.size();
    }
    return result;
}


template<typename CharT>
const basic_string<CharT>
str_replace(const basic_string<CharT>& search,
            const basic_string<CharT>& with,
            const basic_string<CharT>& where)
{
    basic_string<CharT> result;
    for(unsigned a=0; a < where.size(); )
    {
        unsigned b = where.find(search, a);
        if(b == where.npos)
        {
            //result += where.substr(a);
            result.insert(result.end(), where.begin()+a, where.end());
            break;
        }
        //result += where.substr(a, b-a);
        result.insert(result.end(), where.begin()+a, where.begin()+b);
        result += with;
        a = b + search.size();
    }
    return result;
}

template<typename CharT>
void
str_replace_inplace(basic_string<CharT>& where,
                    const basic_string<CharT>& search,
                    CharT with)
{
    for(typename basic_string<CharT>::size_type a = where.size();
        (a = where.rfind(search, a)) != where.npos;
        where.replace(a, search.size(), 1, with));
}

template<typename CharT>
void
str_replace_inplace(basic_string<CharT>& where,
                    const basic_string<CharT>& search,
                    const basic_string<CharT>& with)
{
    for(typename basic_string<CharT>::size_type a = where.size();
        (a = where.rfind(search, a)) != where.npos;
        where.replace(a, search.size(), with));
}

#ifndef ctMiscFunHH
#define ctMiscFunHH

#include <string>

using std::basic_string;

template<typename CharT>
const basic_string<CharT>
str_replace(const basic_string<CharT>& search,
            CharT with,
            const basic_string<CharT>& where);

template<typename CharT>
const basic_string<CharT>
str_replace(const basic_string<CharT>& search,
            const basic_string<CharT>& with,
            const basic_string<CharT>& where);

extern const char *mempos(const char *haystack,unsigned haysize,
                          const char *needle,  unsigned needlesize);
extern unsigned hashstr(const char *s, unsigned len);




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
            result.insert(result.end(), where.begin()+a, where.end());
            //result += where.substr(a);
            break;
        }
        result.insert(result.end(), where.begin()+a, where.begin()+b);
        //result += where.substr(a, b-a);
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
            result.insert(result.end(), where.begin()+a, where.end());
            //result += where.substr(a);
            break;
        }
        result.insert(result.end(), where.begin()+a, where.begin()+b);
        //result += where.substr(a, b-a);
        result.insert(result.end(), with.begin(), with.end());
        //result += with;
        a = b + search.size();
    }
    return result;
}

#endif

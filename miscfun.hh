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

template<typename CharT>
void
str_replace_inplace(basic_string<CharT>& where,
                    const basic_string<CharT>& search,
                    CharT with);

template<typename CharT>
void
str_replace_inplace(basic_string<CharT>& where,
                    const basic_string<CharT>& search,
                    const basic_string<CharT>& with);

#include "miscfun.tcc"

#endif

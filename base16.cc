#include "base16.hh"
#include "miscfun.hh"

const std::string EncodeBase16(unsigned n, unsigned min_digits)
{
    return format("%*X", min_digits, n);
}

bool CumulateBase16(unsigned& value, char c)
{
    if(c >= '0' && c <= '9')
        value=value*16 + (c-'0');
    else if(c >= 'A' && c <= 'F')
        value=value*16 + (10 + c-'A');
    else if(c >= 'a' && c <= 'f')
        value=value*16 + (10 + c-'a');
    else
        return false;
    return true;
}

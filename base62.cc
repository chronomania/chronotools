#include "base62.hh"

const std::string EncodeBase62(unsigned n, unsigned min_digits)
{
    std::string result;
    while(n > 0)
    {
        unsigned dig = n%62;
        char ch;
        if(dig < 10) ch = '0' + dig;
        else if(dig < 36) ch = 'A' + (dig-10);
        else ch = 'a' + (dig-36);
        
        result.insert(result.begin(), ch);
        
        n /= 62;
    }
    while(result.size() < min_digits) result.insert(result.begin(), '0');
    return result;
}

const bool CumulateBase62(unsigned& value, char c)
{
    if(c >= '0' && c <= '9')
        value=value*62 + (c-'0');
    else if(c >= 'A' && c <= 'Z')
        value=value*62 + (10 + c-'A');
    else if(c >= 'a' && c <= 'z')
        value=value*62 + (36 + c-'a');
    else
        return false;
    return true;
}

#include <boost/regex.hpp>

/* Example mask:
 *    [Goto:%0 [If:%1]]
 */
template<typename CharT>
bool Match(const std::basic_string<CharT>& input,
           const std::basic_string<CharT>& mask,
           std::basic_string<CharT>& p0,
           std::basic_string<CharT>& p1)
{
    std::basic_string<CharT> regexp;
    regexp += '^';
    int has_0=0;
    int has_1=0;
    for(unsigned a=0; a<mask.size(); ++a)
        switch((wchar_t)mask[a])
        {
            case L'%':
            {
                ++a;
                if(mask[a] != L'%')
                {
                    if(mask[a] == L'0')
                    {
                        has_0 = has_1+1; // select 1 or 2
                        regexp += '(';
                        regexp += '.';
                        regexp += '*';
                        regexp += ')';
                        break;
                    }
                    if(mask[a] == L'1')
                    {
                        has_1 = has_0+1; // select 1 or 2
                        regexp += '(';
                        regexp += '.';
                        regexp += '*';
                        regexp += ')';
                        break;
                    }
                    --a;
                }
                goto plain;
            }
            case L'[': case L'\\': case L']':
            case L'^': case L'$':
            case L'{': case L'}': case L',':
            case L'(': case L')':
            case L'.': case L'*': case L'?':
                regexp += '\\'; //PASSTHRU
            default:
            plain:
                regexp += mask[a];
        }
    regexp += '$';

    boost::match_results<typename std::basic_string<CharT>::const_iterator> what;
    boost::basic_regex<CharT> exp(regexp);
    
    if(!boost::regex_match(input, what, exp)) return false;
    
    if(has_0)p0.assign(what[has_0].first, what[has_0].second);
    if(has_1)p1.assign(what[has_1].first, what[has_1].second);
    return true;
}

#ifndef WIN32
template<typename CharT>
void RegexReplace(const std::basic_string<CharT>& pattern,
                  const std::basic_string<CharT>& replacement,
                  std::basic_string<CharT>& subject)
{
    boost::basic_regex<CharT> exp(pattern);
    subject = boost::regex_replace(subject, exp, replacement, boost::format_sed);
}
#endif

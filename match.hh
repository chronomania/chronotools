#include <string>

/* Example mask:
 *    [Goto:%0 [If:%1]]
 */
template<typename CharT>
bool Match(const std::basic_string<CharT>& input,
           const std::basic_string<CharT>& mask,
           std::basic_string<CharT>& p0,
           std::basic_string<CharT>& p1);

template<typename CharT>
inline bool Match(const std::basic_string<CharT>& input,
                  const CharT* const mask,
                  std::basic_string<CharT>& p0,
                  std::basic_string<CharT>& p1)
{
    return Match(input, std::basic_string<CharT>(mask), p0, p1);
}

template<typename CharT>
inline bool Match(const std::basic_string<CharT>& input,
                  const std::basic_string<CharT>& mask,
                  std::basic_string<CharT>& p0)
{
    std::basic_string<CharT> p1; return Match(input,mask,p0,p1);
}

template<typename CharT>
inline bool Match(const std::basic_string<CharT>& input,
                  const CharT* const mask,
                  std::basic_string<CharT>& p0)
{
    std::basic_string<CharT> p1; return Match(input,mask,p0,p1);
}

template<typename CharT>
inline bool Match(const std::basic_string<CharT>& input,
                  const std::basic_string<CharT>& mask)
{
    std::basic_string<CharT> p0, p1; return Match(input,mask,p0,p1);
}

template<typename CharT>
inline bool Match(const std::basic_string<CharT>& input,
                  const CharT* const mask)
{
    std::basic_string<CharT> p0,p1; return Match(input,mask,p0,p1);
}

/* Pattern: regex
 * Replacement: backreferences as \\1 and such
 */
template<typename CharT>
void RegexReplace(const std::basic_string<CharT>& pattern,
                  const std::basic_string<CharT>& replacement,
                  std::basic_string<CharT>& subject);

template<typename CharT>
inline void RegexReplace(const CharT* pattern,
                         const CharT* replacement,
                         std::basic_string<CharT>& subject)
{
    RegexReplace(std::basic_string<CharT>(pattern),
                 std::basic_string<CharT>(replacement),
                 subject);
}

#include "match.tcc"

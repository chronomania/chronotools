#include "scriptfile.hh"
#include "compress.hh"
#include "strload.hh"
#include "symbols.hh"
#include "msgdump.hh"
#include "rommap.hh"
#include "config.hh"

#include "dumptext.hh"

#include <cstdarg>

namespace
{
    std::wstring dict_converted[256];
    ctstring dict_unconverted[256];
}

void LoadDict(unsigned offs, unsigned len)
{
    vector<ctstring> strings = LoadPStrings(offs, len, L"dictionary(p)");
    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        const ctstring &s = strings[a];
        
        dict_unconverted[a + 0x21] = s;
        
        std::wstring tmp(s.size(), 0);
        for(unsigned b=0; b<s.size(); ++b)
            tmp[b] = getwchar_t(s[b], cset_12pix);
        dict_converted[a + 0x21] = tmp;
    }
}

void DumpDict()
{
    StartBlock(L"d", L"dictionary");

    for(unsigned a=0; a<256; ++a)
    {
        const std::wstring &s = dict_converted[a];
        if(s.empty()) continue;

        PutBase16Label(a);
        PutContent(s + L';', false);
    }
    
    EndBlock();
}

const std::wstring Disp8Char(ctchar k)
{
    const Symbols::revtype &symbols = Symbols.GetRev(2);
    
    Symbols::revtype::const_iterator i = symbols.find(k);
    
    if(i != symbols.end())
        return i->second;

    if(k == 0x2D) return L":";
        
    wchar_t tmp = getwchar_t(k, cset_8pix);
    if(tmp == ilseq)
    {
        return wformat(L"[%02X]", k);
    }
    
    std::wstring result;
    result += tmp;
    return result;
}

const std::wstring Disp12Char(ctchar k)
{
    // Override these special ones to get proper formatting:
    switch(k)
    {
        /*
            [nl] is a linefeed.
            [pausenl] is a pause, then linefeed.
            [cls] is a screen clearing.
            [pause] is a pause, then clear screen.
        */
    
        case 0x05: return L"[nl]\n";
        case 0x06: return L"[nl]\n   ";
        case 0x07: return L"[pausenl]\n";
        case 0x08: return L"[pausenl]\n   ";
        case 0x09: return L"\n[cls]\n";
        case 0x0A: return L"\n[cls]\n   ";
        case 0x0B: return L"\n[pause]\n";
        case 0x0C: return L"\n[pause]\n   ";
    }
    
    const Symbols::revtype &symbols = Symbols.GetRev(16);
    
    Symbols::revtype::const_iterator i = symbols.find(k);
    
    if(i != symbols.end())
        return i->second;

    // Note:
    // The character names
    // (Crono,Marle,Lucca,Robo,Frog,Ayla,Magus,Nadia,Epoch)
    // are quite obfuscated in the ROM. We use hardcoded
    // symbol map instead of trying to decipher the ROM.
    
    if(k < 256 && !dict_converted[k].empty())
        return dict_converted[k];

    wchar_t tmp = getwchar_t(k, cset_12pix);
    if(tmp == ilseq)
    {
        return wformat(L"[%02X]", k);
    }

    std::wstring result;
    result += tmp;
    return result;
}

namespace
{
    const ctstring AttemptUnwrapParagraph(const ctstring& para)
    {
        const ctchar space  = getctchar(' ');
        const ctchar colon  = getctchar(':');
        const ctchar period = getctchar('.');
        const ctchar que    = getctchar('?');
        const ctchar excl   = getctchar('!');
        const ctchar lquo   = getctchar((unsigned char)'«');
        const ctchar rquo   = getctchar((unsigned char)'»');
        
        ctstring space3(3, space);
        
        bool line_firstword = true;
        bool first_line     = true;
        bool indented       = para.substr(0, 3) == space3;
        bool had_delimiter  = false;
        
        const Symbols::revtype &symbols = Symbols.GetRev(16);
        
        ctstring result = para;
        for(unsigned a=0; a<result.size(); ++a)
        {
            ctchar k = result[a];
            if(k < 256 && !dict_unconverted[k].empty())
            {
                result.replace(a, 1, dict_unconverted[k]);
                k = result[a];
            }
            
            if(k == 0x05 /* nl */
            || k == 0x07 /* pausenl */)
            {
                /* FIXME: This is not a good check for $6q2o and similar */
                if(result[a+1] == lquo)
                {
                    had_delimiter = true;
                }
                
                if(result.find(space3 + space3, a+1) != result.npos)
                {
                    /* Abort wrapping - there's preformatted text following. */
                    return result;
                }
                
                if(!had_delimiter && k != 0x07 && !line_firstword)
                {
                    if(indented && result.substr(a+1, 3) == space3)
                    {
                        result.replace(a, 4, 1, space);
                        continue;
                    }
                    if(!indented && result.substr(a+1, 3) != space3)
                    {
                        result.replace(a, 1, 1, space);
                        continue;
                    }
                }
                line_firstword = true;
                first_line = false;
                indented = result.substr(a+1, 3) == space3;
                continue;
            }
            
            had_delimiter =
                k == colon || k == period
             || k == que || k == excl
                 /* FIXME: Not a good check against "..." */
             || (k >= 0x21 && symbols.find(k) != symbols.end())
                 /* FIXME: Not a good check against $8SFk and similar */
             || k == 0x0D || k == 0x0E || k == 0x0F
             || k == rquo
                 ;
            
            if(k == colon && line_firstword)
                indented = true;

            if(k == space || k == colon)
                line_firstword = false;
        }
        return result;
    }

    void AttemptUnwrap(ctstring& line)
    {
        const bool Attempt = GetConf("dumper", "attempt_unwrap");
        if(!Attempt) return;
        
        ctstring space3(3, getctchar(' '));
        
        /* Replace all [nl3] with [nl] + 3 spaces and so on */
        for(unsigned a=0; a<line.size(); ++a)
        {
            switch(line[a])
            {
                case 0x06: line[a] = 0x05; break;
                case 0x08: line[a] = 0x07; break;
                case 0x0A: line[a] = 0x09; break;
                case 0x0C: line[a] = 0x0B; break;
                default: continue;
            }
            line.insert(a+1, space3);
            a += space3.size();
        }
        
        ctstring result;
        unsigned parabegin = 0;
        for(unsigned a=0; a<=line.size(); ++a)
        {
            if(a == line.size()
            || line[a] == 0x09 /* cls */
            || line[a] == 0x0B /* pause */)
            {
                result += AttemptUnwrapParagraph(line.substr(parabegin, a-parabegin));
                if(a < line.size()) result += line[a];
                parabegin = a+1;
            }
        }
        line = result;
    }
    
    const std::wstring Get12string(const ctstring& value, bool dolf)
    {
        ctstring s = value;
        std::wstring result;
        
        if(dolf) AttemptUnwrap(s);

        for(unsigned b=0; b<s.size(); ++b)
        {
            switch(s[b])
            {
                case 0x03:
                {
                    result += wformat(L"[delay %02X]", (unsigned char)s[++b]);
                    break;
                }
                case 0x12:
                {
                    ++b;
                    if(s[b] == 0x00) result += L"[tech]";
                    else if(s[b] == 0x01) result += L"[monster]";
                    else
                    {
                        result += wformat(L"[12][%02X]", (unsigned char)s[b]);
                    }
                    break;
                }
                default:
                {
                    result += Disp12Char(s[b]);
                }
            }
        }
        return result;
    }
    
    const std::wstring Get8string(const ctstring& value)
    {
        ctstring s = value;
        std::wstring result;
        
        unsigned attr=0;
        for(unsigned b=0; b<s.size(); ++b)
        {
            
Retry:
            switch(s[b])
            {
#if 1
                case 1:
                {
                    result += L"[next]";
                    break;
                }
                case 2:
                {
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    result += wformat(L"[goto,%04X]", c);
                    break;
                }
                case 3:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    result += wformat( L"[func1,%04X,%04X]", c1,c2);
                    break;
                }
                case 4:
                {
                    unsigned c1, c2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c1 = (unsigned char)s[++b];

#if 1
                    if(c1 >= 0x40 && (c1 < 0x7E || c1 >= 0x80))
                    {
                        unsigned addr = (SNES2ROMpage(c1) << 16) + c2;
                        
                        unsigned bytes;
                        ctstring str = LoadZString(addr, bytes, L"substring", Extras_8);
                        
                        //result += L'{'; //}
                        
                        b -= 3;
                        s.erase(b, 4);
                        s.insert(b, str);
                        goto Retry;
                    }
#endif
                    
                    result += wformat(L"[substr,%02X%04X]", c1,c2);
                    break;
                }
                case 5:
                {
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    result += wformat(L"[member,%04X]", c);
                    break;
                }
                case 6:
                {
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    result += wformat(L"[attrs,%04X]", c);
                    break;
                }
                case 7:
                {
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    result += wformat(L"[out,%04X]", c);
                    break;
                }
                case 8:
                {
                    unsigned c = (unsigned char)s[++b];
                    result += wformat(L"[spc,%02X]", c);
                    break;
                }
                case 9:
                {
                    unsigned c = (unsigned char)s[++b];
                    result += wformat(L"[len,%02X]", c);
                    break;
                }
                case 10:
                {
                    unsigned c = (unsigned char)s[++b];
                    attr = c;
                    result += wformat(L"[attr,%02X]", c);
                    break;
                }
                case 11:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    result += wformat(L"[func2,%04X,%04X]", c1,c2);
                    break;
                }
                case 12:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[++b];
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    result += wformat(L"[stat,%02X,%04X]", c1,c2);
                    break;
                }
#endif
                default:
                {
#if 1
                    if(attr & 0x03)
                    {
                        result += L"[gfx";
                        while(b < s.size())
                        {
                            unsigned char byte = s[b];
                            if(byte <= 12) { --b; break; }

                            result += wformat(L",%02X", byte);
                            ++b;
                        }
                        result += L']';
                    }
                    else
                    {
                        result += (Disp8Char(s[b]));
                    }
#else
                    result += wformat(L"[%02X]", s[b]);
#endif
                }
            }
        }
        return result;
    }
}

void DumpZStrings(const unsigned offs,
                  const std::wstring& what,
                  unsigned len,
                  bool dolf)
{
    MessageBeginDumpingStrings(offs);
    
    const std::wstring what_tab = what + L"(z)";
    
    vector<ctstring> strings = LoadZStrings(offs, len, what_tab, Extras_12);

    StartBlock(L"z", what);

    for(unsigned a=0; a<strings.size(); ++a)
    {
        std::wstring line = Get12string(strings[a], dolf);

        PutBase62Label(offs + a*2);
        
        PutContent(line, dolf);
    }
    
    EndBlock();
    MessageDone();
}

void DumpMZStrings(const unsigned offs,
                   const std::wstring& what,
                   unsigned len,
                   bool dolf)
{
    MessageBeginDumpingStrings(offs);
    
    const std::wstring what_tab = what + L"(Z)";
    
    vector<ctstring> strings = LoadZStrings(offs, len, what_tab, Extras_12);

    StartBlock(L"z", what + L" (change this to *Z to allow free relocation)"); 

    for(unsigned a=0; a<strings.size(); ++a)
    {
        std::wstring line = Get12string(strings[a], dolf);
        PutBase62Label(offs + a*2);
        PutContent(line, dolf);
    }
    
    EndBlock();
    MessageDone();
}

void DumpRZStrings(const std::wstring& what,
                   unsigned len,
                   bool dolf,
                   ...)
{
    unsigned pageaddr = 0;
    unsigned offsaddr = 0;
    
    std::wstring label = L"z";
    
    va_list ap;
    va_start(ap, dolf);
    
    for(;;)
    {
        char format = va_arg(ap, int);
        if(!format) break;
        label += L':';
        label += AscToWchar(format);
        unsigned addr = va_arg(ap, unsigned);
        label += Base62Label(addr);
        if(format == '^') pageaddr = addr;
        if(format == '!') offsaddr = addr;
    }
    va_end(ap);

    unsigned offs = (ROM[pageaddr  ] << 16)
                  | (ROM[offsaddr+1] << 8)
                  | (ROM[offsaddr  ]);
    offs = SNES2ROMaddr(offs);
    
    MessageBeginDumpingStrings(offs);
    
    const std::wstring what_tab = what + L"(zr)";
    
    vector<ctstring> strings = LoadZStrings(offs, len, what_tab, Extras_12);

    StartBlock(label, what);

    for(unsigned a=0; a<strings.size(); ++a)
    {
        std::wstring line = Get12string(strings[a], dolf);
        PutBase62Label(offs + a*2);
        PutContent(line, dolf);
    }
    
    EndBlock();
    MessageDone();
}

void DumpC8String(const unsigned ptroffs,
                  const std::wstring& what)
{
    unsigned offs = (ROM[ptroffs+2 ] << 16)
                  | (ROM[ptroffs+1 ] << 8)
                  | (ROM[ptroffs+0 ]);
    offs = SNES2ROMaddr(offs);
    
    MessageBeginDumpingStrings(offs);
    
    vector<unsigned char> Buffer(65536);
    unsigned orig_bytes = Uncompress(ROM+offs, Buffer, ROM+GetROMsize());
    unsigned new_bytes = Buffer.size();
    
    MarkFree(offs, orig_bytes, what);
    
    MessageBeginDumpingStrings(offs);
    
    StartBlock(L"c", what);
    
    /* The hardcoded numbers between 8A..B8
     * are for the character names that are
     * in the compressed block that comes
     * originally from address DB0000.
     */
    
    PutBase62Label(ptroffs);
    std::wstring line;
    unsigned l=0;
    for(unsigned a=0; a<new_bytes; ++a)
    {
        unsigned l0 = line.size();
        
        ctchar k = Buffer[a];
        
        bool special_region = a >= 0x89 && a <= 0xB9;
        
        wchar_t tmp = ilseq;
        if(special_region) tmp = getwchar_t(k, cset_8pix);
        if(tmp == ilseq)
        {
            if(k < 10 && special_region)
                line += wformat(L"[%X]", k);
            else
                line += wformat(L"[%02X]", k);
        }
        else
            line += tmp;

        l += line.size()-l0;
        bool wrap = l>=64;
        if(special_region && (a%6 == 5)) wrap=true;
        if(wrap) { line += L"\n"; l=0; }
    }
    PutContent(line, true);
    
    EndBlock();

    MessageDone();
}

void Dump8Strings(const unsigned offs,
                  const std::wstring& what,
                  unsigned len)
{
    MessageBeginDumpingStrings(offs);
    
    const std::wstring what_tab = what + L"(r)";
    
    vector<ctstring> strings = LoadZStrings(offs, len, what_tab, Extras_8);

    StartBlock(L"r", what);

    for(unsigned a=0; a<strings.size(); ++a)
    {
        std::wstring line = Get8string(strings[a]);
        PutBase62Label(offs + a*2);
        PutContent(line, true);
    }
    EndBlock();

    MessageDone();
}

void DumpFStrings(unsigned offs,
                  const std::wstring& what,
                  unsigned len,
                  unsigned maxcount)
{
    MessageBeginDumpingStrings(offs);
    
    const std::wstring what_tab = what + L"(f)";
    
    vector<ctstring> strings = LoadFStrings(offs, len, what_tab, maxcount);
    
    StartBlock(L"l%u", what, len);

    for(unsigned a=0; a<strings.size(); ++a)
    {
        const ctstring &s = strings[a];

        std::wstring line;
        for(unsigned b=0; b<s.size(); ++b)
            line += Disp8Char(s[b]);

        PutBase62Label(offs + a*len);
        PutContent(line, false);
    }
    EndBlock();
    MessageDone();
}

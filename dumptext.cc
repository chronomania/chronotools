#include "scriptfile.hh"
#include "strload.hh"
#include "symbols.hh"
#include "msgdump.hh"
#include "rommap.hh"
#include "config.hh"

#include "dumptext.hh"

namespace
{
    ucs4string dict_converted[256];
    ctstring dict_unconverted[256];
}

void LoadDict(unsigned offs, unsigned len)
{
    vector<ctstring> strings = LoadPStrings(offs, len, "dictionary(p)");
    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        const ctstring &s = strings[a];
        
        dict_unconverted[a + 0x21] = s;
        
        ucs4string tmp(s.size(), 0);
        for(unsigned b=0; b<s.size(); ++b)
            tmp[b] = getucs4(s[b], cset_12pix);
        dict_converted[a + 0x21] = tmp;
    }
}

void DumpDict()
{
    StartBlock("d", "dictionary");

    wstringOut conv(getcharset());

    for(unsigned a=0; a<256; ++a)
    {
        const ucs4string &s = dict_converted[a];
        if(s.empty()) continue;

        string line = conv.puts(s);
        line += conv.putc(';');
        
        PutBase16Label(a);
        PutContent(line, false);
    }
    
    EndBlock();
}

const ucs4string Disp8Char(ctchar k)
{
    const Symbols::revtype &symbols = Symbols.GetRev(2);
    
    Symbols::revtype::const_iterator i = symbols.find(k);
    
    if(i != symbols.end())
        return i->second;

    if(k == 0x2D) return AscToWstr(":");
        
    ucs4 tmp = getucs4(k, cset_8pix);
    if(tmp == ilseq)
    {
        char Buf[32];
        sprintf(Buf, "[%02X]", k);
        return AscToWstr(Buf);
    }
    
    ucs4string result;
    result += tmp;
    return result;
}

const ucs4string Disp12Char(ctchar k)
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
    
        case 0x05: return AscToWstr("[nl]\n");
        case 0x06: return AscToWstr("[nl]\n   ");
        case 0x07: return AscToWstr("[pausenl]\n");
        case 0x08: return AscToWstr("[pausenl]\n   ");
        case 0x09: return AscToWstr("\n[cls]\n");
        case 0x0A: return AscToWstr("\n[cls]\n   ");
        case 0x0B: return AscToWstr("\n[pause]\n");
        case 0x0C: return AscToWstr("\n[pause]\n   ");
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

    ucs4 tmp = getucs4(k, cset_12pix);
    if(tmp == ilseq)
    {
        char Buf[32];
        sprintf(Buf, "[%02X]", k);
        return AscToWstr(Buf);
    }

    ucs4string result;
    result += tmp;
    return result;
}

namespace
{
    const ctstring AttemptUnwrapParagraph(const ctstring& para)
    {
        const ctchar space  = getchronochar(' ');
        const ctchar colon  = getchronochar(':');
        const ctchar period = getchronochar('.');
        const ctchar que    = getchronochar('?');
        const ctchar excl   = getchronochar('!');
        const ctchar lquo   = getchronochar((unsigned char)'«');
        const ctchar rquo   = getchronochar((unsigned char)'»');
        
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
        
        ctstring space3(3, getchronochar(' '));
        
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
    
    const ucs4string Get12string(const ctstring& value, bool dolf)
    {
        ctstring s = value;
        ucs4string result;
        
        if(dolf) AttemptUnwrap(s);

        for(unsigned b=0; b<s.size(); ++b)
        {
            switch(s[b])
            {
                case 0x03:
                {
                    char Buf[64];
                    sprintf(Buf, "[delay %02X]", (unsigned char)s[++b]);
                    result += AscToWstr(Buf);
                    break;
                }
                case 0x12:
                {
                    char Buf[64];
                    ++b;
                    if(s[b] == 0x00)
                        strcpy(Buf, "[tech]");
                    else if(s[b] == 0x01)
                        strcpy(Buf, "[monster]");
                    else
                        sprintf(Buf, "[12][%02X]", (unsigned char)s[b]);
                    result += AscToWstr(Buf);
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
    
    const ucs4string Get8string(const ctstring& value)
    {
        ctstring s = value;
        ucs4string result;
        
        unsigned attr=0;
        for(unsigned b=0; b<s.size(); ++b)
        {
            char Buf[64];
Retry:
            switch(s[b])
            {
#if 1
                case 1:
                {
                    result += (AscToWstr("[next]"));
                    break;
                }
                case 2:
                {
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[goto,%04X]", c);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 3:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[func1,%04X,%04X]", c1,c2);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 4:
                {
                    unsigned c1, c2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c1 = (unsigned char)s[++b];

#if 1
                    if(c1 >= 0xC0)
                    {
                        unsigned addr = ((c1&0x3F) << 16) + c2;
                        
                        unsigned bytes;
                        ctstring str = LoadZString(addr, bytes, "substring", Extras_8);
                        
                        //result += (AscToWchar('{')); //}
                        
                        b -= 3;
                        s.erase(b, 4);
                        s.insert(b, str);
                        goto Retry;
                    }
#endif
                    
                    sprintf(Buf, "[substr,%02X%04X]", c1,c2);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 5:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[member,%04X]", c);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 6:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[attrs,%04X]", c);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 7:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[out,%04X]", c);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 8:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[spc,%02X]", c);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 9:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[len,%02X]", c);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 10:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[attr,%02X]", c);
                    result += (AscToWstr(Buf));
                    attr = c;
                    break;
                }
                case 11:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[func2,%04X,%04X]", c1,c2);
                    result += (AscToWstr(Buf));
                    break;
                }
                case 12:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[++b];
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[stat,%02X,%04X]", c1,c2);
                    result += (AscToWstr(Buf));
                    break;
                }
#endif
                default:
#if 1
                    if(attr & 0x03)
                    {
                        result += (AscToWstr("[gfx"));
                        while(b < s.size())
                        {
                            unsigned char byte = s[b];
                            if(byte <= 12) { --b; break; }

                            char Buf[32];
                            sprintf(Buf, ",%02X", byte);
                            result += (AscToWstr(Buf));
                            ++b;
                        }
                        result += (AscToWchar(']'));
                    }
                    else
                    {
                        result += (Disp8Char(s[b]));
                    }
#else
                    sprintf(Buf, "[%02X]", s[b]);
                    result += (AscToWstr(Buf));
#endif
            }
        }
        return result;
    }
}

void DumpZStrings(const unsigned offs,
                  const string& what,
                  unsigned len,
                  bool dolf)
{
    MessageBeginDumpingStrings(offs);
    
    const string what_tab = what+"(z)";
    
    vector<ctstring> strings = LoadZStrings(offs, len, what_tab, Extras_12);

    StartBlock("z", what); 

    wstringOut conv(getcharset());    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        ucs4string line = Get12string(strings[a], dolf);

        PutBase62Label(offs + a*2);
        
        PutContent(conv.puts(line), dolf);
    }
    
    EndBlock();
    MessageDone();
}

void DumpRZStrings(const unsigned pageaddr,
                   const unsigned offsaddr,
                   const string& what,
                   unsigned len,
                   bool dolf)
{
    const unsigned offs = ((ROM[pageaddr   & 0x3FFFFF] << 16)
                         | (ROM[offsaddr+1 & 0x3FFFFF] << 8)
                         | (ROM[offsaddr   & 0x3FFFFF])) & 0x3FFFFF;
    
    MessageBeginDumpingStrings(offs);
    
    const string what_tab = what+"(zr)";
    
    vector<ctstring> strings = LoadZStrings(offs, len, what_tab, Extras_12);

    string label = "z";
    label += ":^"; label += Base62Label(pageaddr);
    label += ":!"; label += Base62Label(offsaddr);

    StartBlock(label, what); 

    wstringOut conv(getcharset());    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        ucs4string line = Get12string(strings[a], dolf);

        PutBase62Label(offs + a*2);
        
        PutContent(conv.puts(line), dolf);
    }
    
    EndBlock();
    MessageDone();
}

void Dump8Strings(const unsigned offs,
                  const string& what,
                  unsigned len)
{
    MessageBeginDumpingStrings(offs);
    
    const string what_tab = what+"(r)";
    
    vector<ctstring> strings = LoadZStrings(offs, len, what_tab, Extras_8);

    StartBlock("r", what);

    wstringOut conv(getcharset());    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        ucs4string line = Get8string(strings[a]);
        PutBase62Label(offs + a*2);
        PutContent(conv.puts(line), true);
    }
    EndBlock();

    MessageDone();
}

void DumpFStrings(unsigned offs,
                  const string& what,
                  unsigned len,
                  unsigned maxcount)
{
    MessageBeginDumpingStrings(offs);
    
    const string what_tab = what+"(f)";
    
    vector<ctstring> strings = LoadFStrings(offs, len, what_tab, maxcount);
    
    StartBlock("l%u", what, len);

    wstringOut conv(getcharset());

    for(unsigned a=0; a<strings.size(); ++a)
    {
        ucs4string line;
        
        const ctstring &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
            line += Disp8Char(s[b]);

        PutBase62Label(offs + a*len);
        PutContent(conv.puts(line), false);
    }
    EndBlock();
    MessageDone();
}

#include "scriptfile.hh"
#include "strload.hh"
#include "symbols.hh"
#include "msgdump.hh"

#include "dumptext.hh"

static ucs4string dict[256];

void LoadDict(unsigned offs, unsigned len)
{
    vector<ctstring> strings = LoadPStrings(offs, len, "dictionary(p)");
    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        const ctstring &s = strings[a];
        ucs4string tmp;
        for(unsigned b=0; b<s.size(); ++b)tmp += getucs4(s[b], cset_12pix);
        dict[a + 0x21] = tmp;
    }
}

void DumpDict()
{
    StartBlock("d", "dictionary");

    wstringOut conv(getcharset());

    for(unsigned a=0; a<256; ++a)
    {
        const ucs4string &s = dict[a];
        
        if(s.empty()) continue;

        string line;

        for(unsigned b=0; b<s.size(); ++b)
            line += conv.putc(s[b]);

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
    
    if(k < 256 && !dict[k].empty())
        return dict[k];

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
        string line;
        const ctstring &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
        {
            switch(s[b])
            {
                case 0x03:
                {
                    char Buf[64];
                    sprintf(Buf, "[delay %02X]", (unsigned char)s[++b]);
                    line += conv.puts(AscToWstr(Buf));
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
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                default:
                {
                    line += conv.puts(Disp12Char(s[b]));
                }
            }
        }

        PutBase62Label(offs + a*2);
        PutContent(line, dolf);
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
        string line;
        
        ctstring s = strings[a];

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
                    line += conv.puts(AscToWstr("[next]"));
                    break;
                }
                case 2:
                {
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[goto,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 3:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[func1,%04X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
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
                        
                        //line += conv.putc(AscToWchar('{')); //}
                        
                        b -= 3;
                        s.erase(b, 4);
                        s.insert(b, str);
                        goto Retry;
                    }
#endif
                    
                    sprintf(Buf, "[substr,%02X%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 5:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[member,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 6:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[attrs,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 7:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[out,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 8:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[spc,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 9:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[len,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 10:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[attr,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    attr = c;
                    break;
                }
                case 11:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[func2,%04X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 12:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[++b];
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[stat,%02X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
#endif
                default:
#if 1
                    if(attr & 0x03)
                    {
                        line += conv.puts(AscToWstr("[gfx"));
                        while(b < s.size())
                        {
                            unsigned char byte = s[b];
                            if(byte <= 12) { --b; break; }

                            char Buf[32];
                            sprintf(Buf, ",%02X", byte);
                            line += conv.puts(AscToWstr(Buf));
                            ++b;
                        }
                        line += conv.putc(AscToWchar(']'));
                    }
                    else
                    {
                        line += conv.puts(Disp8Char(s[b]));
                    }
#else
                    sprintf(Buf, "[%02X]", s[b]);
                    line += conv.puts(AscToWstr(Buf));
#endif
            }
        }
        PutBase62Label(offs + a*2);
        PutContent(line, false);
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
        string line;
        
        const ctstring &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
            line += conv.puts(Disp8Char(s[b]));

        PutBase62Label(offs + a*len);
        PutContent(line, false);
    }
    EndBlock();
    MessageDone();
}

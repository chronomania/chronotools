#include <cstdio>
#include <exception>

using namespace std;

#include "ctinsert.hh"
#include "ctcset.hh"
#include "config.hh"
#include "symbols.hh"
#include "rommap.hh"
#include "rom.hh"
#include "conjugate.hh"
#include "typefaces.hh"

namespace
{
    void PutC(unsigned char c, std::FILE* fp)
    {
        // 8-bit output.
        std::fputc(c, fp);
    }

    void PutL(unsigned int w, std::FILE* fp)
    {
        // 24-bit msb-first IPS output.
        PutC((w >> 16) & 0xFF, fp);
        PutC((w >> 8) & 0xFF, fp);
        PutC(w & 0xFF, fp);
    }

    void PutMW(unsigned short w, std::FILE* fp)  
    {
        // 16-bit msb-first IPS output.
        PutC(w >> 8,  fp);
        PutC(w & 0xFF, fp);
    }                      

    void PutS(const void* s, unsigned n, std::FILE* fp)
    {
        std::fwrite(s, n, 1, fp);
        //for(unsigned a=0; a<n; ++a) PutC(s[a], fp);
    }

    void GeneratePatch(class ROM &ROM, unsigned offset, const char *fn)
    {
        fprintf(stderr, "Creating %s\n", fn);
        
        unsigned MaxHunkSize = GetConf("patch",   "maxhunksize");

        /* Now write the patch */
        FILE *fp = fopen(fn, "wb");
        if(!fp)
        {
            perror(fn);
            return;
        }
        fwrite("PATCH", 1, 5, fp);
        
        unsigned addr = 0;
        for(;;)
        {
            unsigned size;
            addr = ROM.FindNextBlob(addr, size);
            if(!size) break;

            for(unsigned left = size; left > 0; )
            {
                unsigned count = MaxHunkSize;
                if(count > left) count = left;
                
                /*
                fprintf(stderr, "Writing %u(of %u) @ %06X\n", count, left, addr);
                */
                
                if(addr == IPS_EOF_MARKER)
                {
                    fprintf(stderr,
                        "Error: IPS doesn't allow patches that go to $%X\n", addr);
                }
                else if(addr == IPS_ADDRESS_EXTERN)
                {
                    fprintf(stderr,
                        "Error: Address $%X is reserved for IPS_ADDRESS_EXTERN\n", addr);
                }
                else if(addr == IPS_ADDRESS_GLOBAL)
                {
                    fprintf(stderr,
                        "Error: Address $%X is reserved for IPS_ADDRESS_GLOBAL\n", addr);
                }
                else if(addr > 0xFFFFFF)
                {
                    fprintf(stderr,
                        "Error: Address $%X is too big for IPS format\n", addr);
                }
                
                PutL((addr + offset) & 0x3FFFFF, fp);
                PutMW(count, fp);
                
                std::vector<unsigned char> data = ROM.GetContent(addr, count);
                
                PutS(&data[0], count, fp);
                
                left -= count;
                addr += count;
            }
        }
        fwrite("EOF",   1, 3, fp);
        fclose(fp);
    }
    void GeneratePatches(class ROM &ROM)
    {
        const string Name = WstrToAsc(GetConf("general", "gamename"));
        
        /* Set game name */
        vector<unsigned char> Buf(21, ' ');
        for(unsigned a=0; a < Buf.size() && a < Name.size(); ++a)
            Buf[a] = Name[a];
        
        ROM.AddPatch(Buf, 0xFFC0, "game name");
        
        /* Should there be a language code at $FFB5? */
        /* No - it would affect PAL/NTSC things */
        
        GeneratePatch(ROM, 0,   WstrToAsc(GetConf("patch", "patchfn_nohdr")).c_str());
        GeneratePatch(ROM, 512, WstrToAsc(GetConf("patch", "patchfn_hdr")).c_str());
        
        ShowProtMap2();
    }
}

const string DispString(const ctstring &s, unsigned symbols_type)
{
    const Symbols::revtype &symbols = Symbols.GetRev(symbols_type);
    
    static wstringOut conv(getcharset());

    string result;
    for(unsigned a=0; a<s.size(); ++a)
    {
        ctchar c = s[a];

        Symbols::revtype::const_iterator i = symbols.find(c);
        if(i != symbols.end())
        {
            result += conv.puts(i->second);
            continue;
        }

        ucs4 u = ilseq;
        switch(symbols_type)
        {
            case 16: u = getucs4(c, cset_12pix); break;
            case  8: u = getucs4(c, cset_8pix); break;
            case  2: u = getucs4(c, cset_8pix); break;
        }
        
        if(u != ilseq)
        {
            result += conv.putc(u);
            continue;
        }

        char Buf[8];
        sprintf(Buf, "[%02X]", c);
        result += conv.puts(AscToWstr(Buf));
    }
    return result;
}

void insertor::ReportFreeSpace()
{
    freespace.Report();
}

insertor::insertor(): Conjugater(NULL)
{
}

insertor::~insertor()
{
    delete Conjugater; Conjugater = NULL;
}

int main(void)
{
    std::set_terminate (__gnu_cxx::__verbose_terminate_handler);

    fprintf(stderr,
        "Chrono Trigger script insertor version "VERSION"\n"
        "Copyright (C) 1992,2004 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    insertor *ins = new insertor;
    
    const string font8fn  = WstrToAsc(GetConf("font",   "font8fn"));
    const string font8vfn = WstrToAsc(GetConf("font",   "font8vfn"));
    const string font12fn = WstrToAsc(GetConf("font",   "font12fn"));
    const string scriptfn = WstrToAsc(GetConf("readin", "scriptfn"));
    
    LoadTypefaces();
    
    // Font loading must happen before script loading,
    // or script won't be properly paragraph-wrapped.
    ins->LoadFont8(font8fn);
    ins->LoadFont8v(font8vfn);
    ins->LoadFont12(font12fn);
    
    {FILE *fp = fopen(scriptfn.c_str(), "rt");
    if(!fp)
    {
        perror(scriptfn.c_str());
        return -1;
    }
    char Buf[8192];setvbuf(fp, Buf, _IOFBF, sizeof Buf);
    ins->LoadFile(fp);
    fclose(fp);}
    
    ins->ReorganizeFonts();
    
    ins->DictionaryCompress();
    
    ins->ReportFreeSpace();

    fprintf(stderr, "--\n");
    
    ins->WriteEverything();
    
    fprintf(stderr, "--\n");
    
    fprintf(stderr, "Creating a virtual ROM...\n");
    class ROM ROM(4194304);

    ins->PatchROM(ROM);

    fprintf(stderr, "--\n");
    ins->ReportFreeSpace();
    
    fprintf(stderr, "--\n");
    fprintf(stderr, "Unallocating insertor data...\n");
    delete ins; ins = NULL;
    
    GeneratePatches(ROM);
    
    return 0;
}

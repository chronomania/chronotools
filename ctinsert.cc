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

    struct RLE { unsigned addr, len; };

    static RLE FindRLE(const char* source, const unsigned len, unsigned addr)
    {
    /*
        In IPS format:
        Not-RLE: 3+2+N   (5+N)
        RLE:     3+2+2+1 (8)
        
        RLE will only be used, if:
          The length of preceding chunk > A
          The length of successor chunk > B
          The length of the RLE         > 8
        Where
          A = 0
          B = 0
    */

        RLE result;
        result.len=0;
        for(unsigned a=0; a<len; )
        {
            unsigned rle_len = 1;
            
            if(addr+a != IPS_EOF_MARKER
            && addr+a != IPS_ADDRESS_EXTERN
            && addr+a != IPS_ADDRESS_GLOBAL)
            {
                while(a+rle_len < len && source[a+rle_len] == source[a]
                   && rle_len < 0xFFFF) ++rle_len;
            }
            
            //unsigned size_before = a;
            //unsigned size_after  = len-(a+rle_len);
            
            if((addr+a+rle_len == IPS_EOF_MARKER
             || addr+a+rle_len == IPS_ADDRESS_EXTERN
             || addr+a+rle_len == IPS_ADDRESS_GLOBAL)
            && rle_len > 1) { --rle_len; }
            
            if(rle_len <= 8) { a += rle_len; continue; }
            
            result.len  = rle_len;
            result.addr = a;
            break;
        }
        return result;
    }

    static void PutChunk(FILE*fp, unsigned addr, unsigned nbytes, const char* source)
    {
        const unsigned MaxHunkSize = GetConf("patch",   "maxhunksize");
        const bool UseRLE          = GetConf("patch",   "use_rle");

        while(nbytes > 0)
        {
            if(UseRLE)
            {
                RLE rle = FindRLE(source, nbytes, addr);
                if(rle.len)
                {
                    if(rle.addr > 0)
                    {
                        PutChunk(fp, addr, rle.addr, source);
                        addr   += rle.addr;
                        source += rle.addr;
                        nbytes -= rle.addr;
                    }
                    /*
                    fprintf(stderr, "%u times %02X at $%06X\n",
                        rle.len, (unsigned char)*source, addr);
                    */
                    PutL(addr, fp);
                    PutMW(0, fp);
                    PutMW(rle.len, fp);
                    PutC(*source, fp);
                    source += rle.len;
                    addr   += rle.len;
                    nbytes -= rle.len;
                    continue;
                }
            }

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

            unsigned eat = MaxHunkSize;
            if(eat > nbytes) eat = nbytes;
            //fprintf(stderr, "%u bytes hunk at $%06X\n", eat, addr);
            PutL(addr, fp);
            PutMW(eat, fp);
            fwrite(source, eat, 1, fp);
            addr   += eat;
            source += eat;
            nbytes -= eat;
        }
    }
    void GeneratePatch(class ROM& ROM, unsigned offset, const string& fn)
    {
        fprintf(stderr, "Creating %s\n", fn.c_str());
        
        /* Now write the patch */
        FILE *fp = fopen(fn.c_str(), "wb");
        if(!fp)
        {
            perror(fn.c_str());
            return;
        }
        fwrite("PATCH", 1, 5, fp);
        
        unsigned addr = 0;
        for(;;)
        {
            unsigned size;
            addr = ROM.FindNextBlob(addr, size);
            if(!size) break;

            std::vector<unsigned char> data = ROM.GetContent(addr, size);
            PutChunk(fp, addr+offset, data.size(), (const char*)&data[0]);
            addr += data.size();
        }
        fwrite("EOF",   1, 3, fp);
        fclose(fp);
    }
    
    void WriteGameName(class ROM& ROM)
    {
        const string Name = GetConf("general", "gamename");
        
        /* Set game name */
        vector<unsigned char> Buf(21, ' ');
        for(unsigned a=0; a < Buf.size() && a < Name.size(); ++a)
            Buf[a] = Name[a];
        
        ROM.AddPatch(Buf, 0xFFC0, L"game name");
    }
    
    void WriteROMsize(class ROM& ROM)
    {
        unsigned size = GetROMsize();
        unsigned sizebyte = 0;
        for(sizebyte=0; (1 << sizebyte)*1024U < size; ++sizebyte) {}
        
        if(size >= 0x600000)
        {
            ROM.Write(0xFFD5, 0x35, L"rom speed&type tag");
            ROM.Write(size-1, 0xFF, L"EOF");
        }
        ROM.Write(0xFFD7, sizebyte, L"rom size tag");
    }
    
    void GeneratePatches(class ROM& ROM)
    {
        WriteGameName(ROM);
        WriteROMsize(ROM);
        
        /* Should there be a language code at $FFB5? */
        /* No - it would affect PAL/NTSC things */
        
        GeneratePatch(ROM, 0,   GetConf("patch", "patchfn_nohdr"));
        GeneratePatch(ROM, 512, GetConf("patch", "patchfn_hdr"));
        
        ShowProtMap2();
    }
}

const std::string DispString(const ctstring &s, unsigned symbols_type)
{
    const Symbols::revtype &symbols = Symbols.GetRev(symbols_type);
    
    std::wstring result;
    for(unsigned a=0; a<s.size(); ++a)
    {
        ctchar c = s[a];

        Symbols::revtype::const_iterator i = symbols.find(c);
        if(i != symbols.end())
        {
            result += i->second;
            continue;
        }

        switch(symbols_type)
        {
            case 16: result += getwchar_t(c, cset_12pix); break;
            case  8: result += getwchar_t(c, cset_8pix); break;
            case  2: result += getwchar_t(c, cset_8pix); break;
            default: result += wformat(L"[%02X]", c); break;
        }
    }
    wstringOut conv;
    conv.SetSet(getcharset());
    return conv.puts(result);
}

void insertor::ReportFreeSpace()
{
    freespace.Report();
}

insertor::insertor()
   : objects(),
     strings(),
     refers(),
     table_counter(),
     dict(),
     Font8(),
     Font8v(),
     Font12(),
     freespace(),
     Conjugater(NULL)
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
        "Copyright (C) 1992,2005 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    insertor *ins = new insertor;
    
    /* Ensure the ROM size */
    ins->ExpandROM();
    
    const string font8fn  = GetConf("font",   "font8fn");
    const string font8vfn = GetConf("font",   "font8vfn");
    const string font12fn = GetConf("font",   "font12fn");
    const string scriptfn = GetConf("readin", "scriptfn");
    
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

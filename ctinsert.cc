#include <cstdio>
#include <exception>

using namespace std;

#include "ctinsert.hh"
#include "ctcset.hh"
#include "config.hh"
#include "symbols.hh"
#include "rom.hh"
#include "conjugate.hh"
#include "typefaces.hh"

namespace
{
    void GeneratePatch(ROM &ROM, unsigned offset, const char *fn)
    {
        fprintf(stderr, "Creating %s\n", fn);
        
        unsigned MaxHunkSize = GetConf("patch", "maxhunksize");

        /*
        {FILE *fp = fopen("chrono-uncompressed.smc", "rb");
        if(fp){fseek(fp,512,SEEK_SET);fread(&ROM[0], 1, ROM.size(), fp);fclose(fp);}}
        */
        
        /* Now write the patch */
        FILE *fp = fopen(fn, "wb");
        if(!fp)
        {
            perror(fn);
            return;
        }
        fwrite("PATCH", 1, 5, fp);

        /* Format:   24bit offset, 16-bit size, then data; repeat */
        for(unsigned a=0; a<ROM.size(); ++a)
        {
            if(!ROM.touched(a))continue;
            
            putc(((a+offset)>>16)&255, fp);
            putc(((a+offset)>> 8)&255, fp);
            putc(((a+offset)    )&255, fp);
            
            unsigned offs=a, c=0;
            while(a < ROM.size() && ROM.touched(a) && c < MaxHunkSize)
                ++c, ++a;
            
            // Size is "c" in both.
            putc((c>> 8)&255, fp);
            putc((c    )&255, fp);
            int ret = fwrite(&ROM[offs], 1, c, fp);
            if(ret < 0 || ret != (int)c)
            {
                fprintf(stderr, " fwrite failed: %d != %d - this patch will be broken.\n", ret, c);
                perror("fwrite");
            }
        }
        fwrite("EOF",   1, 3, fp);
        fclose(fp);
    }
    void GeneratePatches(ROM &ROM)
    {
        GeneratePatch(ROM, 0,   WstrToAsc(GetConf("patch", "patchfn_nohdr")).c_str());
        GeneratePatch(ROM, 512, WstrToAsc(GetConf("patch", "patchfn_hdr")).c_str());
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
        "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n");
    
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
    char Buf[8192];setvbuf(fp, Buf, _IOFBF, sizeof Buf);
    ins->LoadFile(fp);
    fclose(fp);}
    
    ins->ReorganizeFonts();
    
    ins->DictionaryCompress();
    
    /////////ins->GenerateCode();
    
    ins->ReportFreeSpace();

    fprintf(stderr, "Creating a virtual ROM...\n");
    ROM ROM(4194304);
    
    ins->LoadImage("FIN/active1.tga", 0x3FF008);
    ins->LoadImage("FIN/active2.tga", 0x3FF488);
    ins->LoadImage("FIN/elem1.tga",   0x3FD5E4 + 0*2*6*32);
    ins->LoadImage("FIN/elem2.tga",   0x3FD5E4 + 1*2*6*32);
    ins->LoadImage("FIN/elem3.tga",   0x3FD5E4 + 2*2*6*32);
    ins->LoadImage("FIN/elem4.tga",   0x3FD5E4 + 3*2*6*32);

    ins->LoadImage("FIN/face1.tga",   0x3F0000 + 0*6*6*32);
    ins->LoadImage("FIN/face2.tga",   0x3F0000 + 1*6*6*32);
    ins->LoadImage("FIN/face3.tga",   0x3F0000 + 2*6*6*32);
    ins->LoadImage("FIN/face4.tga",   0x3F0000 + 3*6*6*32);
    ins->LoadImage("FIN/face5.tga",   0x3F0000 + 4*6*6*32);
    ins->LoadImage("FIN/face6.tga",   0x3F0000 + 5*6*6*32);
    ins->LoadImage("FIN/face7.tga",   0x3F0000 + 6*6*6*32);
    // Palettes comes after this??
    ins->LoadImage("FIN/face8.tga",   0x3FEB88); // Epoch

    ins->PatchROM(ROM);

    ins->ReportFreeSpace();
    
    fprintf(stderr, "Unallocating insertor data...\n");
    delete ins; ins = NULL;
    
    GeneratePatches(ROM);
    
    return 0;
}

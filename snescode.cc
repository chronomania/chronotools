#include <list>
#include <string>
#include <cstdarg>

#include "o65.hh"
#include "config.hh"
#include "images.hh"
#include "tgaimage.hh"
#include "compress.hh"
#include "settings.hh"
#include "refer.hh"
#include "rommap.hh"
#include "ctinsert.hh"
#include "logfiles.hh"

void insertor::PlaceByte(unsigned char byte,
                         unsigned address,
                         const std::wstring& what)
{
    vector<unsigned char> Buf(1, byte);
    objects.AddLump(Buf, address, WstrToAsc(what));
}

void insertor::ObsoleteCode(unsigned address, unsigned bytes, bool barrier)
{
    const bool ClearSpace = GetConf("patch", "clear_free_space");
    
    // 0x80 = BRA
    // 0xEA = NOP
    if(barrier)
    {
        /* If the code is unreachable, the memory area can simply be freed */
        if(bytes > 0)
        {
            freespace.Add(address, bytes);
            if(ClearSpace)
            {
                vector<unsigned char> empty(bytes, 0);
                objects.AddLump(empty, address, "free space");
            }
            bytes = 0;
        }
    }
    else
    {
        /* Code is reachable, thus we generate some jumps over it. */
        
        /* If the range is big, we create BRLs. */
        while(bytes > 127+3)
        {
            bytes -= 3;
            unsigned skipbytes = bytes;
            if(skipbytes > 32767) skipbytes = 32767;
            
            vector<unsigned char> brl(3);
            brl[0] = 0x82;
            brl[1] = skipbytes & 255;
            brl[2] = skipbytes >> 8;
            objects.AddLump(brl, address, "brl"); // BRL over the space.
            address += 3;
            
            freespace.Add(address, skipbytes);
            if(ClearSpace)
            {
                vector<unsigned char> empty(skipbytes, 0);
                objects.AddLump(empty, address, "free space");
            }
            
            bytes -= skipbytes;
        }
        
        /* If the range is small, we create BRAs. */
        while(bytes > 2)
        {
            bytes -= 2;
            unsigned skipbytes = bytes;
            if(skipbytes > 127) skipbytes = 127;
            
            vector<unsigned char> bra(2);
            bra[0] = 0x80;
            bra[1] = skipbytes;
            objects.AddLump(bra, address, "bra"); // BRA over the space.
            address += 2;
            
            freespace.Add(address, skipbytes);
            if(ClearSpace)
            {
                vector<unsigned char> empty(skipbytes, 0);
                objects.AddLump(empty, address, "free space");
            }
            
            bytes -= skipbytes;
        }
        
        /* If the range is too big for BRAs, we create NOPs. */
        vector<unsigned char> nops(bytes, 0xEA);
        if(!nops.empty())
            objects.AddLump(nops, address, "nop");
    }
}

#include <cerrno>

struct Image
{
    TGAimage image;

    wstring tab_sym;
    wstring pal_sym;
    wstring palsize_sym;

    vector<unsigned char> ImgData;
    vector<unsigned char> Palette;
    unsigned OriginalSize;
    
    Image(const TGAimage& img,
          const wstring& tabsym,
          const wstring& palsym,
          const wstring& palsizesym)
      : image(img), tab_sym(tabsym), pal_sym(palsym), palsize_sym(palsizesym),
        ImgData(), Palette(), OriginalSize()
    {
    }
    
    bool Error() const { return image.Error(); }
    
    void MakePalette()
    {
        unsigned palsize = image.GetPalSize();
        // palette index 0 is never used.
        for(unsigned a=1; a<palsize; ++a)
        {
            unsigned value = image.GetPalEntry(a);
            
            Palette.push_back(value & 255);
            Palette.push_back(value >> 8);
        }
    }
    
    void MakeData(unsigned segment)
    {
        vector<unsigned char> uncompressed;
        LoadImageData(image, uncompressed);
        OriginalSize = uncompressed.size();
        ImgData = Compress(&uncompressed[0], uncompressed.size(), segment);
    }
};


void insertor::WriteUserCode()
{
    if(true) /* load code */
    {
        const ConfParser::ElemVec& elems = GetConf("linker", "load_code").Fields();
        for(unsigned a=0; a<elems.size(); a += 1)
        {
            const wstring& codefn  = elems[a];
            const string filename = WstrToAsc(codefn);
            const string what = filename;
            
            FILE *fp = fopen(filename.c_str(), "rb");
            if(!fp)
            {
                if(errno != ENOENT)
                {
                    string message = "> ";
                    message += what;
                    message += " disabled: ";
                    message += filename;
                    perror(message.c_str());
                }
            }
            else
            {
                char Buf[5] = "";
                fread(Buf, 5, 1, fp);
                rewind(fp);
                
                if(std::equal(Buf, Buf+5, "PATCH"))
                {
                    objects.LoadIPSfile(fp, what);
                }
                else
                {
                    O65 code;
                    code.Load(fp);
                    if(!code.Error())
                    {
                        objects.AddObject(code, what);
                    }
                }
                fclose(fp);
            }
        }
    }
    
    if(true) /* load images */
    {
        const ConfParser::ElemVec& elems = GetConf("linker", "add_image").Fields();
        for(unsigned a=0; a<elems.size(); a += 5)
        {
            const wstring& imagefn     = elems[a];
            const unsigned segment        = elems[a+1];
            const wstring& tab_sym     = elems[a+2];
            const wstring& pal_sym     = elems[a+3];
            const wstring& palsize_sym = elems[a+4];
        
            const string filename = WstrToAsc(imagefn);

            Image img(filename, tab_sym, pal_sym, palsize_sym);
            if(img.Error())
            {
                continue;
            }
            
            img.MakeData(segment);
            img.MakePalette();
            
            objects.DefineSymbol(WstrToAsc(img.palsize_sym), img.Palette.size());

            objects.AddLump(img.ImgData, filename+" data",    WstrToAsc(img.tab_sym));
            objects.AddLump(img.Palette, filename+" palette", WstrToAsc(img.pal_sym));
        }
    }
    
    if(true) /* link calls */
    {
        const ConfParser::ElemVec& elems = GetConf("linker", "add_call_of").Fields();
        for(unsigned a=0; a<elems.size(); a += 4)
        {
            const wstring& funcname = elems[a];
            unsigned address           = elems[a+1];
            unsigned nopcount          = elems[a+2];
            bool add_rts               = elems[a+3];
            
            /* Convert a SNES address to ROM address */
            address = SNES2ROMaddr(address);
            
            if(!funcname.empty())
            {
                objects.AddReference(WstrToAsc(funcname), CallFrom(address));
                address += 4;
            }
            
            bool barrier = false;
            if(add_rts)
            {
                // 0x60 = RTS
                vector<unsigned char> rts(1, 0x60);
                objects.AddLump(rts, address, "rts");
                ++address;
                barrier = true;
            }
            
            ObsoleteCode(address, nopcount, barrier);
        }
    }

    objects.DefineSymbol("DECOMPRESS_FUNC_ADDR", ROM2SNESaddr(GetConst(DECOMPRESSOR_FUNC_ADDR)));
    objects.DefineSymbol("CHAR_OUTPUT_FUNC",     ROM2SNESaddr(GetConst(DIALOG_DRAW_FUNC_ADDR)));
}


/* This file isn't really the right place for this... */
void insertor::ExpandROM()
{
    unsigned NumPages = GetROMsize() / 0x10000;
    
    for(unsigned page=0x40; page<NumPages; ++page)
    {
        unsigned begin = 0;
        unsigned end   = 0x10000;
        
        if(page == 0x40)
        {
            /* $008000 has to be mirrored at $408000.
             * Thus only the first half is "free".
             */
            end = 0x8000;
        }
        
        unsigned snespage = ROM2SNESpage(page);
        if(snespage == 0x7E
        || snespage == 0x7F
        || snespage == 0x00)
        {
            /* These pages are no use. */
            continue;
        }

        if(snespage < 0x40)
        {
#if 0
            /* For pages that show at $00..$3F, only the high part is accessible. */
            begin = 0x8000;
#else    /* BROKEN */
            continue;
#endif
        }


        if(page == NumPages-1 && end == 0x10000)
        {
            /* Leave one byte for EOF marker */
            --end;
        }
        
        if(end > begin)
        {
            //fprintf(stderr, "%02X:%04X-%04X\n", page,begin,end-1);
            freespace.Add(page, begin, end);
        }
    }
}

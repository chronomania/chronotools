#include <list>
#include <string>
#include <cstdarg>

#include "o65.hh"
#include "config.hh"
#include "images.hh"
#include "tgaimage.hh"
#include "compress.hh"
#include "settings.hh"
#include "config.hh"
#include "refer.hh"
#include "ctinsert.hh"
#include "logfiles.hh"

void insertor::PlaceByte(unsigned char byte,
                         unsigned address,
                         const string& what)
{
    vector<unsigned char> Buf(1, byte);
    objects.AddLump(Buf, address, what);
}

void insertor::ObsoleteCode(unsigned address, unsigned bytes, bool barrier)
{
    const bool ClearSpace = GetConf("patch", "clear_free_space");
    
    address &= 0x3FFFFF;
    
    // 0x80 = BRA
    // 0xEA = NOP
    if(barrier)
    {
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
        
        vector<unsigned char> nops(bytes, 0xEA);
        if(!nops.empty())
            objects.AddLump(nops, address, "nop");
    }
}

#include <cerrno>

struct Image
{
    TGAimage image;

    ucs4string tab_sym;
    ucs4string pal_sym;
    ucs4string palsize_sym;

    vector<unsigned char> ImgData;
    vector<unsigned char> Palette;
    unsigned OriginalSize;
    
    Image(const TGAimage& img,
          const ucs4string& tabsym,
          const ucs4string& palsym,
          const ucs4string& palsizesym)
      : image(img), tab_sym(tabsym), pal_sym(palsym), palsize_sym(palsizesym)
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
            const ucs4string& codefn  = elems[a];
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
            const ucs4string& imagefn     = elems[a];
            const unsigned segment        = elems[a+1];
            const ucs4string& tab_sym     = elems[a+2];
            const ucs4string& pal_sym     = elems[a+3];
            const ucs4string& palsize_sym = elems[a+4];
        
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
            objects.AddLump(img.ImgData, filename+" palette", WstrToAsc(img.pal_sym));
        }
    }
    
    if(true) /* link calls */
    {
        const ConfParser::ElemVec& elems = GetConf("linker", "add_call_of").Fields();
        for(unsigned a=0; a<elems.size(); a += 4)
        {
            const ucs4string& funcname = elems[a];
            unsigned address           = elems[a+1];
            unsigned nopcount          = elems[a+2];
            bool add_rts               = elems[a+3];
            
            objects.AddReference(WstrToAsc(funcname), CallFrom(address));
            
            address += 4;
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

    objects.DefineSymbol("DECOMPRESS_FUNC_ADDR", 0xC00000 | GetConst(DECOMPRESSOR_FUNC_ADDR));
    objects.DefineSymbol("CHAR_OUTPUT_FUNC",     0xC00000 | GetConst(DIALOG_DRAW_FUNC_ADDR));

    objects.AddLump(Font8v.GetWidths(), "vwf8 widths",  "WIDTH_ADDR");
    objects.AddLump(Font8v.GetTiles(),  "vwf8 tiles",   "TILEDATA_ADDR");
}

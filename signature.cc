#include "images.hh"
#include "ctinsert.hh"
#include "compress.hh"
#include "tgaimage.hh"
#include "settings.hh"
#include "config.hh"
#include "o65.hh"

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

void insertor::GenerateSignatureCode()
{
    const ucs4string& codefn  = GetConf("signature", "file");

    const string codefile = WstrToAsc(codefn);
    
    O65 sig_code = LoadObject(codefile, "Signature");
    if(sig_code.Error()) return;

    if(!LinkCalls("signature"))
    {
        fprintf(stderr, "> > Signature won't be used\n");
        return;
    }
    
    const ConfParser::ElemVec& elems = GetConf("signature", "add_image").Fields();
    for(unsigned a=0; a<elems.size(); a += 5)
    {
        const ucs4string& imagefn     = elems[a];
        const unsigned segment        = elems[a+1];
        const ucs4string& tab_sym     = elems[a+2];
        const ucs4string& pal_sym     = elems[a+3];
        const ucs4string& palsize_sym = elems[a+4];
    
        Image img(WstrToAsc(imagefn), tab_sym, pal_sym, palsize_sym);
        if(img.Error())
        {
            continue;
        }
        
        img.MakeData(segment);
        img.MakePalette();
        
        objects.DefineSymbol(WstrToAsc(img.palsize_sym), img.Palette.size());

        objects.AddObject(CreateObject(img.ImgData, WstrToAsc(img.tab_sym)), "sig img data");
        objects.AddObject(CreateObject(img.Palette, WstrToAsc(img.pal_sym)), "sig img palette");
    }
    
    objects.AddObject(sig_code, "sig code");

    objects.DefineSymbol("DECOMPRESS_FUNC_ADDR", 0xC00000 | GetConst(DECOMPRESSOR_FUNC_ADDR));
}

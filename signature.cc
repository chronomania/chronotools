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

    vector<Image> images;
    
    const ConfParser::ElemVec& elems = GetConf("signature", "add_image").Fields();
    for(unsigned a=0; a<elems.size(); a += 5)
    {
        const ucs4string& imagefn     = elems[a];
        const unsigned segment        = elems[a+1];
        const ucs4string& tab_sym     = elems[a+2];
        const ucs4string& pal_sym     = elems[a+3];
        const ucs4string& palsize_sym = elems[a+4];
    
        Image img(WstrToAsc(imagefn), tab_sym, pal_sym, palsize_sym);
        if(!img.image.GetXdim())
        {
            fprintf(stderr, "- failed to load '%s'...\n", WstrToAsc(imagefn).c_str());
            continue;
        }
        
        img.MakeData(segment);
        img.MakePalette();
        
        images.push_back(img);
    }
    
    const string codefile = WstrToAsc(codefn);
    
    O65 sig_code;
    {FILE *fp = fopen(codefile.c_str(), "rb");
    if(!fp) { perror(codefile.c_str()); return; }
    sig_code.Load(fp);
    fclose(fp);}

    unsigned code_size = sig_code.GetCodeSize();

    vector<freespacerec> Organization(1);
    Organization[0].len = code_size;
    for(unsigned a=0; a<images.size(); ++a)
    {
        Organization.push_back(images[a].Palette.size());
        Organization.push_back(images[a].ImgData.size());
    }
    
    freespace.OrganizeToAnyPage(Organization);

    const unsigned CodeAddress = Organization[0].pos;

    for(unsigned a=0; a<images.size(); ++a)
    {
        const unsigned PaletteAddr = Organization[1+a*2].pos;
        const unsigned ImgDataAddr = Organization[2+a*2].pos;

        sig_code.LinkSym(WstrToAsc(images[a].tab_sym),     0xC00000 | ImgDataAddr);
        sig_code.LinkSym(WstrToAsc(images[a].pal_sym),     0xC00000 | PaletteAddr);
        sig_code.LinkSym(WstrToAsc(images[a].palsize_sym), images[a].Palette.size());

        PlaceData(images[a].ImgData, ImgDataAddr);
        PlaceData(images[a].Palette, PaletteAddr);
    }
    
    sig_code.LocateCode(CodeAddress);
    
    sig_code.LinkSym("DECOMPRESS_FUNC_ADDR", 0xC00000 | GetConst(DECOMPRESSOR_FUNC_ADDR));
    
    fprintf(stderr,
        "\r> Signature(%s):"
            " %u(code)@ $%06X,",
        codefile.c_str(),
        code_size, 0xC00000 | CodeAddress);

    for(unsigned a=0; a<images.size(); ++a)
    {
        const unsigned PaletteAddr = Organization[1+a*2].pos;
        const unsigned ImgDataAddr = Organization[2+a*2].pos;
        
        fprintf(stderr,
            " %u(pal%u)@ $%06X,"
            " %u(img%u,orig %u)@ $%06X",
            images[a].Palette.size(), a+1, 0xC00000 | PaletteAddr,
            images[a].ImgData.size(), a+1,
                   images[a].OriginalSize, 0xC00000 | ImgDataAddr
               );
    }
    fprintf(stderr, "\n");
    
    sig_code.Verify();

    SNEScode tmp(sig_code.GetCode());
    tmp.YourAddressIs(CodeAddress);
    codes.push_back(tmp);
    
    LinkCalls("signature", sig_code);
}

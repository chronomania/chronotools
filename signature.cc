#include "images.hh"
#include "ctinsert.hh"
#include "compress.hh"
#include "tgaimage.hh"
#include "config.hh"
#include "o65.hh"

void insertor::GenerateSignatureCode()
{
    const ucs4string& imagefn = GetConf("signature", "image");
    const ucs4string& codefn  = GetConf("signature", "file");
    unsigned segment          = GetConf("signature", "page");
    
    vector<unsigned char> uncompressed;
    
    const TGAimage image(WstrToAsc(imagefn));
    if(!image.GetXdim())
    {
        fprintf(stderr, "- failed to load '%s'...\n", WstrToAsc(imagefn).c_str());
        return;
    }
    
    LoadImageData(image, uncompressed);
    
    const vector<unsigned char> imgdata =
        Compress(&uncompressed[0], uncompressed.size(), segment);
    const unsigned imgdata_addr = freespace.FindFromAnyPage(imgdata.size());
    
    PlaceData(imgdata, imgdata_addr);
    
    const string codefile = WstrToAsc(codefn);
    
    fprintf(stderr, "\rLoading '%s'...\n", codefile.c_str());
    
    O65 sig_code;
    {FILE *fp = fopen(codefile.c_str(), "rb");
    sig_code.Load(fp);
    fclose(fp);}

    unsigned code_size = sig_code.GetCodeSize();
    unsigned code_addr = freespace.FindFromAnyPage(code_size);
    sig_code.LocateCode(code_addr);
    
    sig_code.LinkSym("TILEDATA_ADDR", imgdata_addr | 0xC00000);
    
    //sig_code.LinkSym("PAL_0", image.GetPalEntry(0)); - 0 never used
    sig_code.LinkSym("PAL_1", image.GetPalEntry(1));
    sig_code.LinkSym("PAL_2", image.GetPalEntry(2));
    sig_code.LinkSym("PAL_3", image.GetPalEntry(3));
    sig_code.LinkSym("PAL_4", image.GetPalEntry(4));
    sig_code.LinkSym("PAL_5", image.GetPalEntry(5));
    sig_code.LinkSym("PAL_6", image.GetPalEntry(6));
    sig_code.LinkSym("PAL_7", image.GetPalEntry(7));
    sig_code.LinkSym("PAL_8", image.GetPalEntry(8));
    sig_code.LinkSym("PAL_9", image.GetPalEntry(9));
    sig_code.LinkSym("PAL_A", image.GetPalEntry(10));
    sig_code.LinkSym("PAL_B", image.GetPalEntry(11));
    sig_code.LinkSym("PAL_C", image.GetPalEntry(12));
    sig_code.LinkSym("PAL_D", image.GetPalEntry(13));
    sig_code.LinkSym("PAL_E", image.GetPalEntry(14));
    sig_code.LinkSym("PAL_F", image.GetPalEntry(15));
    
    fprintf(stderr,
        "\rWriting sig: %u(code)@ $%06X, %u(img,orig %u)@ $%06X\n",
            code_size, code_addr,
            imgdata.size(), uncompressed.size(), imgdata_addr
           );
    
    sig_code.Verify();

    SNEScode tmp(sig_code.GetCode());
    tmp.YourAddressIs(code_addr);
    codes.push_back(tmp);
    
    const ConfParser::ElemVec& elems = GetConf("signature", "add_call_of").Fields();
    for(unsigned a=0; a<elems.size(); a += 2)
    {
        const ucs4string& funcname = elems[a];
        unsigned address           = elems[a+1];
        
        SNEScode tmpcode;
        tmpcode.AddCallFrom(address);
        tmpcode.YourAddressIs(sig_code.GetSymAddress(WstrToAsc(funcname)));
        codes.push_back(tmpcode);
    }
}

#include "images.hh"
#include "ctinsert.hh"
#include "compress.hh"
#include "tgaimage.hh"
#include "config.hh"
#include "o65.hh"

void insertor::GenerateSignatureCode()
{
    const ucs4string& imagefn = GetConf("signature", "image");
    const ucs4string& codefn  = GetConf("signature", "code");
    unsigned call_addr        = GetConf("signature", "call");
    unsigned segment          = GetConf("signature", "page");
    
    vector<unsigned char> uncompressed;
    
    const TGAimage image(WstrToAsc(imagefn));
    LoadImageData(image, uncompressed);
    
    const vector<unsigned char> imgdata =
        Compress(&uncompressed[0], uncompressed.size(), segment);
    const unsigned imgdata_addr = freespace.FindFromAnyPage(imgdata.size());
    
    PlaceData(imgdata, imgdata_addr);
    
    O65 code;
    {FILE *fp = fopen(WstrToAsc(codefn).c_str(), "rb");
    code.Load(fp);
    fclose(fp);}

    unsigned code_size = code.GetCodeSize();
    unsigned code_addr = freespace.FindFromAnyPage(code_size);
    code.LocateCode(code_addr);
    
    code.LinkSym("TILEDATA_SEG",  (imgdata_addr >> 16) | 0xC0);
    code.LinkSym("TILEDATA_OFFS", (imgdata_addr & 0xFFFF));
    
    //code.LinkSym("PAL_0", image.GetPalEntry(0)); - 0 never used
    code.LinkSym("PAL_1", image.GetPalEntry(1));
    code.LinkSym("PAL_2", image.GetPalEntry(2));
    code.LinkSym("PAL_3", image.GetPalEntry(3));
    code.LinkSym("PAL_4", image.GetPalEntry(4));
    code.LinkSym("PAL_5", image.GetPalEntry(5));
    code.LinkSym("PAL_6", image.GetPalEntry(6));
    code.LinkSym("PAL_7", image.GetPalEntry(7));
    code.LinkSym("PAL_8", image.GetPalEntry(8));
    code.LinkSym("PAL_9", image.GetPalEntry(9));
    code.LinkSym("PAL_A", image.GetPalEntry(10));
    code.LinkSym("PAL_B", image.GetPalEntry(11));
    code.LinkSym("PAL_C", image.GetPalEntry(12));
    code.LinkSym("PAL_D", image.GetPalEntry(13));
    code.LinkSym("PAL_E", image.GetPalEntry(14));
    code.LinkSym("PAL_F", image.GetPalEntry(15));
    
    fprintf(stderr,
        "\rSignature: code(%u bytes) at $%06X, img(%u bytes (orig %u)) at $%06X\n",
            code_size, code_addr,
            imgdata.size(), uncompressed.size(), imgdata_addr
           );
    
    code.Verify();

    SNEScode tmp(code.GetCode());
    tmp.YourAddressIs(code_addr);
    tmp.AddCallFrom(call_addr);
    codes.push_back(tmp);
}

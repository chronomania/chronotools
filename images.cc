#include "ctinsert.hh"
#include "tgaimage.hh"
#include "config.hh"
#include "compress.hh"
#include "images.hh"
#include "rom.hh"

void LoadImageData
    (const TGAimage& image,
     vector<unsigned char>& data)
{
    unsigned tiles_x = image.GetXdim() / 8;
    unsigned tiles_y = image.GetYdim() / 8;
    
    const unsigned tilecount = tiles_x * tiles_y;
    const unsigned bitness   = image.GetPalSize()==4 ? 2 : 4;
    
    if(!tilecount) return;
    
    data.resize(tilecount * 8 * bitness);
    
    const unsigned slice = (bitness==1 ? 1 : 2);
    
    for(unsigned a=0; a<tilecount; ++a)
    {
        const unsigned tilex = 8 * (a % tiles_x);
        const unsigned tiley = 8 * (a / tiles_x);
        const unsigned tileoffs = a * 8 * bitness;
        
        for(unsigned ty=tiley,y=0; y<8; ++y,++ty)
        {
            unsigned char byte[8] = {0,0,0,0};
            for(unsigned tx=tilex,x=0; x<8; ++x,++tx)
            {
                unsigned pix = image.Point(tx,ty);
                for(unsigned bit = 0; bit < bitness; ++bit)
                {
                    byte[bit] |= (pix&1) << (7-x);
                    pix >>= 1;
                }
            }
            
            for(unsigned bit=0; bit<bitness; ++bit)
            {
                data[
                   tileoffs
                 + y * slice
                 + (bit&1)
                 + (bit/2)*16
                           ] = byte[bit];
            }
        }
    }
}

void insertor::LoadImage(const string& fn, unsigned address)
{
    const TGAimage image(fn);
    imagedata result;
    
    LoadImageData(image, result.data);
    result.address = address;
    
    fprintf(stderr, "Loading '%s'... at address $%06X\n", fn.c_str(), address);
    
    images.push_back(result);
}


void insertor::LoadAlreadyCompressedImage(const string& fn, unsigned address)
{
    address &= 0x3FFFFF;
    
    FILE *fp = fopen(fn.c_str(), "rb");
    fseek(fp, 0, SEEK_END);
    imagedata result;
    result.data.resize(ftell(fp));
    rewind(fp);
    fread(&result.data[0], 1, result.data.size(), fp);
    fclose(fp);
    result.address = address;
    
    fprintf(stderr, "Loading '%s'... at address $%06X\n", fn.c_str(), address);
    
    images.push_back(result);
}

void insertor::LoadAndCompressImage(const string& fn, unsigned address, unsigned char seg)
{
    const TGAimage image(fn);
    
    vector<unsigned char> uncompressed;
    LoadImageData(image, uncompressed);
    
    address &= 0x3FFFFF;
    
    fprintf(stderr, "Loading '%s'... at address $%06X, compressing", fn.c_str(), address);
    
    imagedata result;
    result.data = Compress(&uncompressed[0], uncompressed.size(), seg);
    result.address = address;
    
    fprintf(stderr, " done (%u bytes)\n", result.data.size());
    
    images.push_back(result);
}

void insertor::LoadAndCompressImageWithPointer
   (const string& fn, unsigned address, unsigned char seg)
{
    const TGAimage image(fn);
    
    vector<unsigned char> uncompressed;
    LoadImageData(image, uncompressed);
    
    address &= 0x3FFFFF;
    
    fprintf(stderr, "Loading '%s'... at address $%06X, compressing", fn.c_str(), address);
    
    vector<unsigned char> compressed = 
        Compress(&uncompressed[0], uncompressed.size(), seg);

    SNEScode code(compressed);
    
    code.AddLongPtrFrom(address);
    
    fprintf(stderr, " done (%u bytes)\n", compressed.size());
    
    codes.push_back(code);
}

void insertor::WriteImages(ROM& ROM) const
{
    fprintf(stderr, "Writing images...\n");
    for(imagelist::const_iterator i = images.begin(); i != images.end(); ++i)
    {
        const imagedata& image = *i;
        
        for(unsigned a=0; a<image.data.size(); ++a)
            ROM.Write(image.address + a, image.data[a]);
    }
}

void insertor::LoadImages()
{
    if(true) /* Load unpacked images */
    {
        const ConfParser::ElemVec& elems = GetConf("images", "putimage").Fields();
        for(unsigned a=0; a<elems.size(); a += 2)
        {
            unsigned address            = elems[a];
            const ucs4string& filename  = elems[a+1];
            
            LoadImage(WstrToAsc(filename), address);
        }
    }
    
    if(true) /* Load packed images */
    {
        const ConfParser::ElemVec& elems = GetConf("images", "packedimage").Fields();
        for(unsigned a=0; a<elems.size(); a += 5)
        {
            unsigned ptr_address       = elems[a];
            unsigned space_address     = elems[a+1];
            unsigned orig_size         = elems[a+2];
            unsigned segment           = elems[a+3];
            const ucs4string& filename = elems[a+4];

            // Add the freespace from the original location
            freespace.Add(space_address >> 16,
                          space_address & 0xFFFF,
                          orig_size);
            LoadAndCompressImageWithPointer(WstrToAsc(filename), ptr_address, segment);
        }
    }
}

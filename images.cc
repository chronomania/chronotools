#include "ctinsert.hh"
#include "tgaimage.hh"
#include "config.hh"
#include "compress.hh"
#include "images.hh"
#include "rom.hh"
#include "msginsert.hh"

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
        
        MessageWorking();

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

void insertor::LoadImages()
{
    MessageLoadingImages();
    
    if(true) /* Load unpacked images */
    {
        const ConfParser::ElemVec& elems = GetConf("images", "putimage").Fields();
        for(unsigned a=0; a<elems.size(); a += 2)
        {
            unsigned address            = elems[a];
            const ucs4string& filename  = elems[a+1];
            
            const string fn = WstrToAsc(filename);
            const TGAimage image(fn);
            
            vector<unsigned char> data;
            LoadImageData(image, data);
            
            MessageLoadingItem(fn);
            
            objects.AddLump(data, address, fn);
        }
    }
    
    if(true) /* Load packed images */
    {
        const ConfParser::ElemVec& elems = GetConf("images", "packedimage").Fields();
        for(unsigned a=0; a<elems.size(); a += 6)
        {
            unsigned ptr_ofs_address   = elems[a];
            unsigned ptr_seg_address   = elems[a+1];
            unsigned space_address     = elems[a+2];
            unsigned orig_size         = elems[a+3];
            unsigned segment           = elems[a+4];
            const ucs4string& filename = elems[a+5];

            space_address &= 0x3FFFFF;
            
            const string fn = WstrToAsc(filename);
            
            // Add the freespace from the original location
            // because the image will be moved anyway
            freespace.Add(space_address & 0x3FFFFF, orig_size);
            
            const TGAimage image(fn);
            vector<unsigned char> data;
            LoadImageData(image, data);
            
            MessageLoadingItem(fn);

            data = Compress(&data[0], data.size(), segment);
            
            //fprintf(stderr, " (%u bytes)\n", data.size());
            
            const string name = fn + " data";
            objects.AddLump(data, fn, name);
            
            if(ptr_seg_address == ptr_ofs_address+2)
            {
                objects.AddReference(name, LongPtrFrom(ptr_ofs_address));
            }
            else
            {
                objects.AddReference(name, OffsPtrFrom(ptr_ofs_address));
                objects.AddReference(name, PagePtrFrom(ptr_seg_address));
            }
        }
    }
    
    MessageDone();
}

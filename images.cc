#include "ctinsert.hh"
#include "tgaimage.hh"
#include "config.hh"
#include "compress.hh"
#include "images.hh"
#include "rom.hh"
#include "rommap.hh"
#include "msginsert.hh"

#include <errno.h>

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

void insertor::WriteImages()
{
    MessageLoadingImages();

    static const char Signature[] =
        " Created with Chronotools \0 "
        " http://iki.fi/bisqwit/source/chronotools.html ";
    vector<unsigned char> data(Signature, Signature+sizeof(Signature));
    objects.AddLump(data, "Chronotools Signature", "_CTSIG");

    if(true) /* Load unpacked images */
    {
        const ConfParser::ElemVec& elems = GetConf("images", "putimage").Fields();
        for(unsigned a=0; a<elems.size(); a += 2)
        {
            unsigned address            = elems[a];
            const wstring& filename  = elems[a+1];

            /* The address must be ROM-based. */

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
        for(size_t a=0; a<elems.size(); a += 5)
        {
            unsigned ptr_ofs_address   = elems[a+0];
            unsigned ptr_seg_address   = elems[a+1];
            unsigned space_address     = elems[a+2];
            unsigned orig_size         = elems[a+3];
            const wstring& filename    = elems[a+4];

            /* The addresses must be ROM-based. */

            const string fn = WstrToAsc(filename);

            // Add the freespace from the original location
            // because the image will be moved anyway
            freespace.Add(space_address, orig_size);

            const TGAimage image(fn);
            vector<unsigned char> data;
            LoadImageData(image, data);

            MessageLoadingItem(fn);

            data = Compress(&data[0], data.size());

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

    if(true) /* Load packed raw blobs (TODO: move this somewhere else) */
    {
        const ConfParser::ElemVec& elems = GetConf("images", "packedblob").Fields();
        for(size_t a=0; a<elems.size(); a += 5)
        {
            unsigned ptr_ofs_address   = elems[a+0];
            unsigned ptr_seg_address   = elems[a+1];
            unsigned space_address     = elems[a+2];
            unsigned orig_size         = elems[a+3];
            const wstring& filename    = elems[a+4];

            /* The addresses must be ROM-based. */

            const string fn = WstrToAsc(filename);

            MessageLoadingItem(fn);

            FILE* fp = fopen(fn.c_str(), "rb");
            if(!fp)
            {
                if(errno == ENOENT)
                {
                    fprintf(stderr, "> %s doesn't exist, ignoring\n", fn.c_str());
                }
                else
                {
                    string message = "> Failed to load ";
                    message += fn;
                    perror(message.c_str());
                }
                continue;
            }
            vector<unsigned char> data;
            for(;;)
            {
                unsigned char Buf[8192];
                size_t r = fread(Buf, 1, sizeof Buf, fp);
                if(r <= 0) break;
                data.insert(data.end(), Buf, Buf+r);
            }

            fclose(fp);

            // Add the freespace from the original location
            // because the image will be moved anyway
            freespace.Add(space_address, orig_size);

            data = Compress(&data[0], data.size());

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

    if(true) /* Load raw blobs (TODO: move this somewhere else) - contrib mziab */
    {
        const ConfParser::ElemVec& elems = GetConf("images", "rawblob").Fields();
        for(size_t a=0; a<elems.size(); a += 5)
        {
            unsigned ptr_ofs_address   = elems[a+0];
            unsigned ptr_seg_address   = elems[a+1];
            unsigned space_address     = elems[a+2];
            unsigned orig_size         = elems[a+3];
            const wstring& filename    = elems[a+4];

            /* The addresses must be ROM-based. */

            const string fn = WstrToAsc(filename);

            MessageLoadingItem(fn);

            FILE* fp = fopen(fn.c_str(), "rb");
            if(!fp)
            {
                if(errno == ENOENT)
                {
                    fprintf(stderr, "> %s doesn't exist, ignoring\n", fn.c_str());
                }
                else
                {
                    string message = "> Failed to load ";
                    message += fn;
                    perror(message.c_str());
                }
                continue;
            }
            vector<unsigned char> data;
            for(;;)
            {
                unsigned char Buf[8192];
                size_t r = fread(Buf, 1, sizeof Buf, fp);
                if(r <= 0) break;
                data.insert(data.end(), Buf, Buf+r);
            }

            fclose(fp);

            // Add the freespace from the original location
            // because the image will be moved anyway
            freespace.Add(space_address, orig_size);

            fprintf(stderr, " (%u bytes)\n", (unsigned) data.size());

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


    if(true) /* Load sprite blobs (TODO: move this somewhere else) - contrib mziab */
    {
        const ConfParser::ElemVec& elems = GetConf("images", "spriteblob").Fields();
        for(size_t a=0; a<elems.size(); a += 4)
        {
            unsigned ptr_ofs_address   = elems[a+0];
            unsigned space_address     = elems[a+1];
            unsigned orig_size         = elems[a+2];
            const wstring& filename    = elems[a+3];

            /* The addresses must be ROM-based. */

            const string fn = WstrToAsc(filename);

            MessageLoadingItem(fn);

            FILE* fp = fopen(fn.c_str(), "rb");
            if(!fp)
            {
                if(errno == ENOENT)
                {
                    fprintf(stderr, "> %s doesn't exist, ignoring\n", fn.c_str());
                }
                else
                {
                    string message = "> Failed to load ";
                    message += fn;
                    perror(message.c_str());
                }
                continue;
            }
            vector<unsigned char> data;
            for(;;)
            {
                unsigned char Buf[8192];
                size_t r = fread(Buf, 1, sizeof Buf, fp);
                if(r <= 0) break;
                data.insert(data.end(), Buf, Buf+r);
            }

            fclose(fp);

            // Add the freespace from the original location
            // because the image will be moved anyway
            freespace.Add(space_address, orig_size);

            fprintf(stderr, " (%u bytes)\n", (unsigned) data.size());

            const string name = fn + " data";
            //objects.AddLump(data, fn, name);

            O65 tmp;
            tmp.LoadSegFrom(CODE, data);
            tmp.DeclareGlobal(CODE, name, 0);
            LinkageWish linkage;
            linkage.SetLinkagePage(0x03);
            objects.AddObject(tmp, fn, linkage);
            objects.AddReference(name, OffsPtrFrom(ptr_ofs_address));
        }
    }

    MessageDone();
}

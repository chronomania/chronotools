#include <cstdio>
#include <cerrno>

using namespace std;

#include "tgaimage.hh"

namespace
{
    void TgaPutB(FILE *fp, unsigned c) { fputc(c, fp); }
    void TgaPutW(FILE *fp, unsigned c) { fputc(c&255, fp); fputc(c >> 8, fp); } 
    void TgaPutP(FILE *fp, unsigned r,unsigned g,unsigned b)
    { TgaPutB(fp,r); TgaPutB(fp,g); TgaPutB(fp,b); }
}

TGAimage::TGAimage(const string &filename)
    : xdim(0), ydim(0), xsize(8), ysize(8), xbox(32)
{
    FILE *fp = fopen(filename.c_str(), "rb");
    if(!fp)
    {
        if(errno == ENOENT)
        {
            fprintf(stderr, "> %s doesn't exist, ignoring\n", filename.c_str());
        }
        else
        {
            string message = "> Failed to load ";
            message += filename;
            perror(message.c_str());
        }
        return;
    }
    
    int idlen = fgetc(fp);
    fgetc(fp); // color map type, should be 1
    fgetc(fp); // image type code, should be 1
    fgetc(fp); fgetc(fp); // palette start
    
    this->palsize = fgetc(fp); palsize += fgetc(fp)*256; 
    
    int palbitness = fgetc(fp); // palette bitness, should be 24
    fgetc(fp); fgetc(fp);
    fgetc(fp); fgetc(fp);

    this->xdim=fgetc(fp); this->xdim += fgetc(fp)*256;
    this->ydim=fgetc(fp); this->ydim += fgetc(fp)*256;
    
    pixbitness = fgetc(fp); // pixel bitness, should be 8
    fgetc(fp); // misc, should be 0
    if(idlen)fseek(fp, idlen, SEEK_CUR);
    if(palsize)
    {
        if(palbitness == 24)
        {
            for(unsigned a=0; a<palsize;++a)
            {
                unsigned B = fgetc(fp);
                unsigned G = fgetc(fp);
                unsigned R = fgetc(fp);
                
                unsigned color   = (B*32/256);
                color = color*32 + (G*32/256);
                color = color*32 + (R*32/256);
                
                color &= 0x7FFF;
                
                palette_in.push_back(color);
            }
        }
        else
            fseek(fp, palsize * ((palbitness+7)/8), SEEK_CUR);
    }
    
    unsigned pixbyteness = pixbitness / 8;

    data.resize(xdim*ydim * pixbyteness);
    
    for(unsigned y=ydim; y-->0; )
        fread(&data[y*xdim*pixbyteness], 1, xdim * pixbyteness, fp);
    
    fclose(fp);
}

TGAimage::TGAimage(unsigned x, unsigned y, unsigned char color)
    : xdim(x), ydim(y),
      palsize(0), pixbitness(8),
      data(x*y, color),
      xsize(8),ysize(18), xbox(32)
{
}

void TGAimage::Save(const string &fn, palettetype paltype, const unsigned *palette)
{
    FILE *fp = fopen(fn.c_str(), "wb");
    if(!fp) { perror(fn.c_str()); return; }
    
    vector<unsigned> FilePalette;
    
    int imagetype = 1;
    
    switch(paltype)
    {
        case pal_6color:
            FilePalette.push_back(0x0000FF); //border
            FilePalette.push_back(0x204050); //background
            FilePalette.push_back(0x000000); //shadow
            FilePalette.push_back(0x5A5A5A); //semishadow
            FilePalette.push_back(0xFFFFFF); //paint
            FilePalette.push_back(0x0A6480); //filler
            break;
        case pal_4color:
            FilePalette.push_back(0x000000); //black
            FilePalette.push_back(0x606060);
            FilePalette.push_back(0xC0C0C0);
            FilePalette.push_back(0xFFFFFF); //white
            break;
        case pal_16color:
            for(unsigned a=0; a<16; ++a)
            {
                if(palette) { FilePalette.push_back(palette[a]); continue; }
                unsigned c = a*255/15;
                FilePalette.push_back(c + 256*c + 65536*c);
            }
            break;
        case pal_256color:
            for(unsigned a=0; a<256; ++a)
            {
                if(palette) { FilePalette.push_back(palette[a]); continue; }
                unsigned c = a;
                FilePalette.push_back(c + 256*c + 65536*c);
            }
            break;
        case pal_none:
            imagetype = 2;
            break;
    }
    
    TgaPutB(fp, 0); // id field len
    TgaPutB(fp, FilePalette.size() > 0); // color map type
    TgaPutB(fp, imagetype); // image type code
    TgaPutW(fp, 0); // FilePalette start
    TgaPutW(fp, FilePalette.size());
    TgaPutB(fp, 24);// FilePalette bitness
    TgaPutW(fp, 0);    TgaPutW(fp, 0);
    TgaPutW(fp, xdim); TgaPutW(fp, ydim);
    TgaPutB(fp, pixbitness); // pixel bitness
    
    int misc = 0;
    if(pixbitness == 32) misc |= 8;
    TgaPutB(fp, misc); //misc
    
    for(unsigned a=0; a<FilePalette.size(); ++a)
    {
        TgaPutB(fp, FilePalette[a] & 255);
        TgaPutB(fp, (FilePalette[a] >> 8) & 255);
        TgaPutB(fp, (FilePalette[a] >> 16) & 255);
    }
    
    unsigned pixbyteness = pixbitness / 8;
    for(unsigned y=ydim; y-->0; )
        fwrite(&data[y*xdim * pixbyteness], 1, xdim * pixbyteness, fp);
    
    fclose(fp);
}

const vector<unsigned char> TGAimage::getbox(unsigned boxnum) const
{
    const unsigned boxposx = (boxnum%xbox) * (xsize+1) + 1;
    const unsigned boxposy = (boxnum/xbox) * (ysize+1) + 1;
    /*
    fprintf(stderr, "fetching box %u (%ux%u @ %ux%u) from %ux%u size image (%u bytes)\n",
        boxnum,
        xsize, ysize,
        boxposx, boxposy,
        xdim, ydim, data.size());*/
    
    vector<unsigned char> result(xsize*ysize);
    unsigned pos=0;
    for(unsigned ny=0; ny<ysize; ++ny)
    {
        unsigned ppos = (boxposy + ny) * xdim + boxposx;
        for(unsigned nx=0; nx<xsize; ++nx)
            result[pos++] = data[ppos++];
    }
    return result;
}

unsigned TGAimage::getboxcount() const
{
    return (ydim-1)*(xdim-1) / (ysize+1) / (xsize+1);
}

void TGAimage::PSet(unsigned x,unsigned y, unsigned value)
{
    unsigned ofs = (y*xdim + x) * (pixbitness / 8);
    for(unsigned tmp = pixbitness; tmp >= 8; tmp -= 8)
    {
        data[ofs++] = value & 255;
        value >>= 8;
    }
}

unsigned TGAimage::Point(unsigned x,unsigned y) const
{
    unsigned ofs = (y*xdim + x) * (pixbitness / 8);
    unsigned result = 0;
    for(unsigned tmp = 0; tmp < pixbitness; tmp += 8)
        result |= (data[ofs++] << tmp);
    return result;
}

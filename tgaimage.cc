#include <cstdio>

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
    if(!fp) { perror(filename.c_str()); return; }
    
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
    
    fgetc(fp); // pixel bitness, should be 8
    fgetc(fp); // misc, should be 0
    if(idlen)fseek(fp, idlen, SEEK_CUR);
    if(palsize)fseek(fp, palsize * ((palbitness+7)/8), SEEK_CUR);
    
    data.resize(xdim*ydim);
    
    for(unsigned y=ydim; y-->0; )
        fread(&data[y*xdim], 1, xdim, fp);
    
    fclose(fp);
}

TGAimage::TGAimage(unsigned x, unsigned y, unsigned char color)
    : xdim(x), ydim(y), data(x*y, color),
      xsize(8),ysize(18), xbox(32)
{
}

void TGAimage::Save(const string &fn, palettetype paltype, const unsigned *palette)
{
    FILE *fp = fopen(fn.c_str(), "wb");
    if(!fp) { perror(fn.c_str()); return; }
    
    vector<unsigned> FilePalette;
    
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
    }
    
    TgaPutB(fp, 0); // id field len
    TgaPutB(fp, 1); // color map type
    TgaPutB(fp, 1); // image type code
    TgaPutW(fp, 0); // FilePalette start
    TgaPutW(fp, FilePalette.size());
    TgaPutB(fp, 24);// FilePalette bitness
    TgaPutW(fp, 0);    TgaPutW(fp, 0);
    TgaPutW(fp, xdim); TgaPutW(fp, ydim);
    TgaPutB(fp, 8); // pixel bitness
    TgaPutB(fp, 0); //misc
    
    for(unsigned a=0; a<FilePalette.size(); ++a)
    {
        TgaPutB(fp, FilePalette[a] & 255);
        TgaPutB(fp, (FilePalette[a] >> 8) & 255);
        TgaPutB(fp, (FilePalette[a] >> 16) & 255);
    }
    
    for(unsigned y=ydim; y-->0; )
        fwrite(&data[y*xdim], 1, xdim, fp);
    
    fclose(fp);
}

const vector<char> TGAimage::getbox(unsigned boxnum) const
{
    const unsigned boxposx = (boxnum%xbox) * (xsize+1) + 1;
    const unsigned boxposy = (boxnum/xbox) * (ysize+1) + 1;
    /*
    fprintf(stderr, "fetching box %u (%ux%u @ %ux%u) from %ux%u size image (%u bytes)\n",
        boxnum,
        xsize, ysize,
        boxposx, boxposy,
        xdim, ydim, data.size());*/
    
    vector<char> result(xsize*ysize);
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

void TGAimage::PSet(unsigned x,unsigned y, unsigned char value)
{
    data[y*xdim + x] = value;
}

unsigned char TGAimage::Point(unsigned x,unsigned y) const
{
    return data[y*xdim + x];
}

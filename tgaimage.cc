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
    int palsize = fgetc(fp); palsize += fgetc(fp)*256; 
    int palbitness = fgetc(fp); // palette bitness, should be 24
    fgetc(fp); fgetc(fp);
    fgetc(fp); fgetc(fp);

    xdim=fgetc(fp); xdim += fgetc(fp)*256;
    ydim=fgetc(fp); ydim += fgetc(fp)*256;
    
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

void TGAimage::Save(const string &fn)
{
    FILE *fp = fopen(fn.c_str(), "wb");
    if(!fp) { perror(fn.c_str()); return; }
    
    TgaPutB(fp, 0); // id field len
    TgaPutB(fp, 1); // color map type
    TgaPutB(fp, 1); // image type code
    TgaPutW(fp, 0); // palette start
    TgaPutW(fp, 6); // palette size
    TgaPutB(fp, 24);// palette bitness
    TgaPutW(fp, 0);    TgaPutW(fp, 0);
    TgaPutW(fp, xdim); TgaPutW(fp, ydim);
    TgaPutB(fp, 8); // pixel bitness
    TgaPutB(fp, 0); //misc
    
    TgaPutP(fp, 255,  0,  0); // border
    TgaPutP(fp,  80, 64, 32); // background
    TgaPutP(fp,   0,  0,  0); // edge
    TgaPutP(fp,  90, 90, 90); // semiedge
    TgaPutP(fp, 255,255,255); // paint
    TgaPutP(fp, 128,100, 10); // filler
    
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

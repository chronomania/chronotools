#include <cstdio>

using namespace std;

#include "tgaimage.hh"

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

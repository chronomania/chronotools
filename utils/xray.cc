#ifndef WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <ggi/ggi.h>

static const unsigned WINDOW_WIDTH  = 1088;
static const unsigned WINDOW_HEIGHT = 768;

static const unsigned TILESPACE_X   = 960;
static const unsigned TILESPACE_Y   = 768;

/*
 Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 
 ROM viewer:
 
 You need: libggi (it's a library like SDL)
 To compile: 
   gcc -Wall -W -pedantic -O2 -o xray xray.c -lggi
 To use:
   xray filename.smc

 * No warranty. You are free to modify this source and to
 * distribute the modified sources, as long as you keep the
 * existing copyright messages intact and as long as you
 * remember to add your own copyright markings.
 * You are not allowed to distribute the program or modified versions
 * of the program without including the source code (or a reference to
 * the publicly available source) and this notice with it.
 
*/

#undef putc
#define putc xray_putc

typedef unsigned char byte;

static ggi_visual_t vis;
static ggi_color pal[256];

static byte font[256][8];

#include "compress.hh"
#include "xray.h"

//dummy, used by compress.o
void MessageWorking() { fprintf(stderr, "."); }

static unsigned SwapPosition
   (unsigned swapwidth,
    unsigned swapheight,
     
    /* Tile number on screen */
    unsigned x,unsigned y,
    
    /* How many tiles horizontally */
    unsigned linewidth,
    
    /* How many bytes is each box */
    unsigned boxsize)
{
    /*********
     *
     * 3,2 means, that
     *
     *    AaBbCcDdEeFfGgHh
     *    IiJjKkLlMmNnOoPp
     *    qqRrSsTtUuVvWwXx
     *
     * is automatically converted to:
     * 
     *    AaDdGgJjMmPpSsVv
     *    BbEeHhKkNnQqTtWw
     *    CcFfIillOoRrUuXx
    */
    
    unsigned result = 0;
    
    unsigned sprite_box_size = swapwidth * swapheight;
    
    unsigned box_horiz = x % swapwidth;
    unsigned box_index = x / swapwidth;
    
    result += box_horiz + box_index * sprite_box_size;
    
    unsigned box_vert  = y % swapheight;
    unsigned box_reset = y / swapheight;
    
    result += (box_vert * swapwidth) + box_reset * (linewidth * swapheight);
    
    return result * boxsize;
}

static void Init(void)
{
    ggiInit();
    vis = ggiOpen(NULL);
    if(!vis || ggiSetGraphMode(vis, WINDOW_WIDTH,WINDOW_HEIGHT, 0,0, GT_16BIT))
        exit(-1);
    memcpy(font, VGAFont, sizeof(font));
}
static void Done(void)
{
    ggiClose(vis);
    ggiExit(); 
}
static void SetPal(void)
{
    int a;
    
    for(a=0; a<15; a++)
        pal[a].r = pal[a].g = pal[a].b = (15-a) * 0xFFFF / 15;
    
    pal[251].r=0x0400; pal[251].g=0x2000;pal[251].b=0x7FFF; /* eof */
    pal[252].r=0x0400; pal[252].g=0x6000;pal[252].b=0xFFFF; /* hexcontent */
    pal[253].r=0x7FFF; pal[253].g=0x2000;pal[253].b=0x0400; /* textcontent */
    pal[254].r=0x0400; pal[254].g=0x7FFF;pal[254].b=0x2000; /* position */
    pal[255].r=0xFFFF; pal[255].g=0x3000;pal[255].b=0xFFFF; /* help */
    
    ggiSetPalette(vis, 0,256, pal);
}
static void LoadPokeFont(FILE *fp)
{
    unsigned n;
    memset(font, 0, sizeof(font));
    font[0][4] = 0x80;
    
    fseek(fp, 0x11A80, SEEK_SET);
    fread(font[0x80], 0x80, 8, fp);
    
    fseek(fp, 0x11E80, SEEK_SET);
    for(n=0; n<16; n++)
    {
        fread(font[0x20+n], 1, 8, fp);
        fseek(fp, 8, SEEK_CUR);
    }
    
    fseek(fp, 0x12488, SEEK_SET);
    for(n=0; n<32; n++)
    {
        fread(font[0x40+n], 1, 8, fp);
        fseek(fp, 8, SEEK_CUR);
    }
    
    fseek(fp, 0x11F80, SEEK_SET);
    for(n=0; n<16; n++)
    {
        fread(font[0x60+n], 1, 8, fp);
        fseek(fp, 8, SEEK_CUR);
    }
}
static void putc(unsigned x,unsigned y, unsigned char ch, unsigned char c)
{
    unsigned ax, ay;
    for(ay=0; ay<8; ay++)
    {
        byte b1 = font[ch][ay];
        for(ax=0; ax<8; ax++)
            ggiPutPixel(vis, x+ax, y+ay, ((b1 >> (7-ax))&1) ? c : 15);
    }
}

static void UncompressDump(FILE *fp)
{
    return;
/*
    long origpos = ftell(fp);
    unsigned char Buf[8192];
    fread(Buf, 1, sizeof Buf, fp);
    
    vector<unsigned char> Result;
    unsigned size = Uncompress(Buf, Result);
*/    
}

static void Disp(const char *s)
{
    FILE *fp = fopen(s, "rb");
    
    unsigned posi=0, bits=16, nes=0;
    unsigned ylimit=8;
    unsigned swapwidth=1, swapheight=1;
    
    if(!fp)return;
    
    static byte merk[50][80];
    static const unsigned PIXC[4] = {0,0x0D00,0x0300,0xF777};
    
Redraw:    
    memset(merk, ' ', sizeof(merk));
    ggiSetGCBackground(vis, 15);
    ggiSetGCForeground(vis, 15);  ggiDrawBox(vis, 0,0, WINDOW_WIDTH,240);

    ggiSetGCForeground(vis, 255); ggiDrawVLine(vis, 32*8, 0, 16*8);
    ggiSetGCForeground(vis, 253); ggiDrawVLine(vis, 32*8+1, 0, 16*8);
    
    ggiSetGCForeground(vis, 255);
    
    ggiPuts(vis,WINDOW_WIDTH-16*8,16+0*10, "a,d = shift");
    ggiPuts(vis,WINDOW_WIDTH-16*8,16+1*10, "arrows=move");
    ggiPuts(vis,WINDOW_WIDTH-16*8,16+2*10, "y =pkmn");
    ggiPuts(vis,WINDOW_WIDTH-16*8,16+3*10, "numpad=tile size");
    ggiPuts(vis,WINDOW_WIDTH-16*8,16+4*10, "b=toggle bits");
    ggiPuts(vis,WINDOW_WIDTH-16*8,16+5*10, "n=toggle nes/gb");
    ggiPuts(vis,WINDOW_WIDTH-16*8,16+6*10,"esc=quit");
    
    for(;;)
    {
        unsigned cx=0, cy=0;
        
        unsigned sxlimit = TILESPACE_X / 8;
        unsigned sylimit = TILESPACE_Y / ylimit;
        
        unsigned widthharppaus = (bits * ylimit + 7) / 8;
        
        fseek(fp, posi, SEEK_SET);
        
        UncompressDump(fp);
        
        for(unsigned sy=0; sy<sylimit; sy++)
            for(unsigned sx=0; sx<sxlimit; sx++)
            {
                byte Buf[(32/*bits*/ * 128/*ylimit*/ + 7) / 8];
                
                unsigned x, y;
                
                unsigned bx = sx * 8;
                unsigned by = sy * ylimit;

                fseek(fp, posi + SwapPosition(swapwidth,swapheight, sx,sy, sxlimit, widthharppaus), SEEK_SET);
                
                if(fread(Buf, widthharppaus, 1, fp) != 1)
                {
                    for(y=0; y<ylimit; y++)
                        for(x=0; x<8; x++)
                            ggiPutPixel(vis, bx+x, by+y, 251);
                }
                else
                {
/*
                    if(cy==0 && cx==0)
                    {
                        ggiSetGCForeground(vis, 253);
                        for(x=0; x<bits; x++)
                        {
                            ggiPutc(vis,
                              (x%8)*18, 
                              193+2*(x&8),
                              "0123456789ABCDEF"[Buf[x]>>4]);
                            ggiPutc(vis,
                              (x%8)*18 + 8,
                              193+2*(x&8),
                              "0123456789ABCDEF"[Buf[x]&15]);
                        }
                    }
                    if(cy < (WINDOW_HEIGHT-16*8-16)/8)
                    {
                        for(y=0; y<bits; y++)
                        {
                            byte c = Buf[y];
                            if(c != merk[cy][cx])
                            {
                                putc(cx*8, 16*8 + (cy)*8, merk[cy][cx]=c, 252);
                            }
                            if(++cx>=WINDOW_WIDTH/8){cx=0;cy++;}
                        }
                    }
*/
                    for(y=0; y<ylimit; y++)
                    {
                        byte b1, b2, b3, b4;
                        
                        if(bits==32)
                        {
                            b1 = Buf[y*2];
                            b2 = Buf[y*2+1];
                            b3 = Buf[y*2+16];
                            b4 = Buf[y*2+17];
                        }
                        else
                        {
                            b1 = Buf[nes?y:y*bits/8];
                            b2 = Buf[nes?y+8:y*bits/8+1];
                            b3 = 0;
                            b4 = 0;
                        }
                        switch(bits)
                        {
                            case 16:
                                for(x=0; x<8; x++)
                                {
                                    ggiPutPixel(vis, bx+x, by+y,
                                        PIXC[(((b1 >> (7-x))&1) | (((b2 >> (7-x))&1)<<1))]
                                    );
                                }
                                break;
                            case 32:
                                for(x=0; x<8; x++)
                                {
                                    unsigned value = 
                                        ((b1 >> (7-x))&1)
                                      | (((b2 >> (7-x))&1)<<1)
                                      | (((b3 >> (7-x))&1)<<2)
                                      | (((b4 >> (7-x))&1)<<3);
                                    
                                    ggiPutPixel(vis, bx+x, by+y,
                                           value*31/15
                                     + 32*(value*63/15)
                                     + 2048*(value*31/15)
                                              );
                                }
                                break;
                            case 8:
                                for(x=0; x<8; x++)
                                    ggiPutPixel(vis, bx+x, by+y, PIXC[((b1 >> (7-x))&1)]);
                        }
                    }
                }
            }
        
        ggiSetGCForeground(vis, 255);
        
        {
          unsigned textpos=WINDOW_WIDTH/8-16;
          ggiPutc(vis,8*textpos++,0, 'B');
          ggiPutc(vis,8*textpos++,0, bits==8?'1':bits==16?'2':'4');
          ggiPutc(vis,8*textpos++,0, 'N');
          ggiPutc(vis,8*textpos++,0, nes+'0');
          ggiPutc(vis,8*textpos++,0, 'Y');
          ggiPutc(vis,8*textpos++,0, '0'+(ylimit/100)%10);
          ggiPutc(vis,8*textpos++,0, '0'+(ylimit/10 )%10);
          ggiPutc(vis,8*textpos++,0, '0'+(ylimit/1  )%10);
          ggiPutc(vis,8*textpos++,0, 'S');
          ggiPutc(vis,8*textpos++,0, '0'+(swapwidth/10)%10);
          ggiPutc(vis,8*textpos++,0, '0'+(swapwidth/1 )%10);
          ggiPutc(vis,8*textpos++,0, ',');
          ggiPutc(vis,8*textpos++,0, '0'+(swapheight/10)%10);
          ggiPutc(vis,8*textpos++,0, '0'+(swapheight/1 )%10);
        }
        
        ggiSetGCForeground(vis, 254);
        {char Buf[8];
         unsigned rom = posi | 0xC00000;
         sprintf(Buf, "%07X", posi);
         
         unsigned textpos=WINDOW_WIDTH-16*8;
         ggiPuts(vis,textpos, 16+8*10, Buf);
          
         sprintf(Buf, "%02X:%04X", rom>>16, rom&0xFFFF);
         ggiPuts(vis,textpos, 16+9*10, Buf);
        }
        
        do {
            switch(ggiGetc(vis))
            {
                case 27: return;
                case 'a': if(posi>0)posi--; break;
                case 'd': posi++; break;
                case GIIK_Down:
                case 's': posi+=sxlimit*widthharppaus; break;
                case GIIK_Up:
                case 'w': if(posi>sxlimit*widthharppaus)posi-=sxlimit*widthharppaus;else posi=0; break;
                case GIIK_PageDown:
                case '.':
                case '':
                case 'v': posi+=sxlimit*widthharppaus*sylimit; break;
                case GIIK_PageUp:
                case ',':
                case '':
                case 'u': if(posi>sxlimit*widthharppaus*sylimit)
                               posi-=sxlimit*widthharppaus*sylimit;
                          else posi=0; break;
                case GIIK_Right:
                case '':
                case 'e': posi+=widthharppaus; break;
                case GIIK_Left:
                case '':
                case 'q': if(posi>widthharppaus)posi-=widthharppaus;
                          else posi=0; break;

                case '8': if(swapheight>1)--swapheight;break;
                case '4': if(swapwidth>1)--swapwidth;break;
                case '2': if(swapheight<99)++swapheight;break;
                case '6': if(swapwidth<99)++swapwidth;break;
                
                case 'b': bits=bits==8?16:bits==16?32:8; break;
                case 'n': nes^=1; break;
                case 'y': LoadPokeFont(fp); /* fall to ^L */
                case '': goto Redraw;
                case '-': if(ylimit>1)--ylimit; break;
                case '+': if(ylimit<128)++ylimit; break;
                
                case '>': posi = (posi+0x3FFF)&~0x3FFF; break;
                case '<': posi=(posi > 0x3FFF)?(posi-0x3FFF)&~0x3FFF:0; break;
            }
        } while(ggiKbhit(vis));
    }
}

int main(int argc, const char *const *argv)
{
    //setenv("GGI_DISPLAY", "x:-noshm", 1);
    
    if(argc<2 || access(argv[argc-1], O_RDONLY))
    {
        printf(
            "Designed for Game Boy, SNES and NES files, but can be used on others as well\n"
            "Copyright (C) 1992,2004 Bisqwit (http://iki.fi/bisqwit/)\n"
            "Usage: xray <filename>\n");
        return 0;
    }
    Init();
    
    SetPal();
    
    Disp(argv[argc-1]);
    
    Done();

    return 0;
}

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef linux
 #include <ggi/ggi.h>
#else
 #include <allegro.h>
#endif

/*
 Copyright (C) 1992,2002 Bisqwit (http://bisqwit.iki.fi/)
 
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

typedef unsigned char byte;

static ggi_visual_t vis;
static ggi_color pal[256];

static byte font[256][8];

#include "xray.h"

static void Init(void)
{
    ggiInit();
    vis = ggiOpen(NULL);
    if(!vis || ggiSetGraphMode(vis, 320,200, 0,0, GT_16BIT))
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
static void Disp(const char *s)
{
    FILE *fp = fopen(s, "rb");
    
    unsigned posi=0, swap=0, bits=16, nes=0;
    int ylimit=8;
    
    if(!fp)return;
    
    static byte merk[50][80];
    static const unsigned PIXC[4] = {0,0x0D00,0x0300,0xF777};
    
Redraw:    
    memset(merk, ' ', sizeof(merk));
    ggiSetGCBackground(vis, 15);
    ggiSetGCForeground(vis, 15); ggiDrawBox(vis, 0,0, 320,200);

    ggiSetGCForeground(vis, 255); ggiDrawVLine(vis, 32*8, 0, 16*8);
    ggiSetGCForeground(vis, 253); ggiDrawVLine(vis, 32*8+1, 0, 16*8);
    
    ggiSetGCForeground(vis, 255);
    ggiPuts(vis,256+6,16, "a,d =");
    ggiPuts(vis,256+6,24, "  shift");
    ggiPuts(vis,256+6,32, "arrows=");
    ggiPuts(vis,256+3,40, " move");
    ggiPuts(vis,256+6,48, "y =");
    ggiPuts(vis,256+6,56, "   pkmn");
    ggiPuts(vis,256+3,64, "p=toggle");
    ggiPuts(vis,256+6,72, "   swap");
    ggiPuts(vis,256+3,80, "b=toggle");
    ggiPuts(vis,256+6,88, "   bits");
    ggiPuts(vis,256+3,96, "n=toggle");
    ggiPuts(vis,256+6,104," nes/gb");
    ggiPuts(vis,256+6,112,"esc=");
    ggiPuts(vis,256+6,120,"   quit");
    
    for(;;)
    {
        int sx, sy;
        int cx=0, cy=0;
        
        int sylimit=128/ylimit;
        
        unsigned widthharppaus = (bits * ylimit + 7) / 8;
        
        fseek(fp, posi, SEEK_SET);
        
        for(sy=0; sy<sylimit; sy++)
            for(sx=0; sx<32; sx++)
            {
                byte Buf[512];
                unsigned x, y;
                
                int bx,by;
                
                switch(swap)
                {
                    case 0:
                        bx = sx;
                        by = sy;
                        break;
                    case 1:
                        bx = (sx&1) | ((sy&1)<<1) | (sx&~3);
                        by = ((sx&2)>>1) | (sy&~1);
                        break;
                    case 2:
                        bx = (sx&15) | ((sy&1)<<4);
                        by = ((sx&16)>>4) | (sy&~1);
                        break;
                    case 3:
                        bx = (sx&7) | ((sy&3)<<3) | (sx&~31);
                        by = ((sx&24)>>3) | (sy&~3);
                        break;
                }
                
                /*  1256 */
                /*  3478 */
                
                bx*=8;
                by*=ylimit;
            
                if(fread(Buf, widthharppaus, 1, fp) != 1)
                    for(y=0; y<ylimit; y++)
                        for(x=0; x<8; x++)
                            ggiPutPixel(vis, bx+x, by+y, 251);
                else
                {
                    if(cy==0 && cx==0)
                    {
                        ggiSetGCForeground(vis, 253);
                        for(x=0; x<bits; x++)
                        {
                            ggiPutc(vis,(x%8)*18,     185+(x&8), "0123456789ABCDEF"[Buf[x]>>4]);
                            ggiPutc(vis,(x%8)*18 + 8, 185+(x&8), "0123456789ABCDEF"[Buf[x]&15]);
                        }
                    }
                    if(cy < (200-16*8-16)/8)
                    {
                        #if 0
                        static const char set1[] =
                           "\r                               "  // 0x00
                            "                                "  // 0x20
                          "'\"gÚÄ¿³³Ã ´Ã´ \t &              ."  // 0x40
                            "$  ID                          -"  // 0x60
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ();:[]"  // 0x80
                            "abcdefghijklmnopqrstuvwxyz‚dlstv"  // 0xA0
                            "                                "  // 0xC0
                            "'PM-''?!.      -$*./,+0123456789"; // 0xE0
                        ggiSetGCForeground(vis, 252);
                        for(y=0; y<bits; y++)
                        {
                            byte c = set1[Buf[y]];
                            if(c != merk[cy][cx])
                                ggiPutc(vis, cx*8, 16*8 + (cy)*7, merk[cy][cx]=c);
                            if(++cx>=320/8){cx=0;cy++;}
                        }
                        #else
                        for(y=0; y<bits; y++)
                        {
                            byte c = Buf[y];
                            if(c != merk[cy][cx])
                                putc(cx*8, 16*8 + (cy)*8, merk[cy][cx]=c, 252);
                            if(++cx>=320/8){cx=0;cy++;}
                        }
                        #endif
                    }
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
                                    ggiPutPixel(vis, bx+x, by+y, PIXC[
                                        ((b1 >> (7-x))&1)
                                      | (((b2 >> (7-x))&1)<<1)
                                      | (((b3 >> (7-x))&1)<<2)
                                      | (((b4 >> (7-x))&1)<<3)]
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
        ggiPutc(vis,320-48+ 0,0, 'P');
        ggiPutc(vis,320-48+ 8,0, swap+'0');
        ggiPutc(vis,320-48+16,0, 'B');
        ggiPutc(vis,320-48+24,0, bits==8?'1':bits==16?'2':'4');
        ggiPutc(vis,320-48+32,0, 'N');
        ggiPutc(vis,320-48+40,0, nes+'0');
        
        ggiSetGCForeground(vis, 254);
        {char Buf[8];
         unsigned rom = posi/0x4000;
         
         sprintf(Buf, "%07X", posi); ggiPuts(vis,256+2,185, Buf);
         
         sprintf(Buf, "%02X:%04X", rom, (posi&0x3FFF) + (rom?0x4000:0));
         ggiPuts(vis,256+2,193, Buf);
        }
        
        do {
            switch(ggiGetc(vis))
            {
                case 27: return;
                case 'a': if(posi>0)posi--; break;
                case 'd': posi++; break;
                case GIIK_Down:
                case 's': posi+=32*widthharppaus; break;
                case GIIK_Up:
                case 'w': if(posi>32*widthharppaus)posi-=32*widthharppaus;else posi=0; break;
                case GIIK_PageDown:
                case '.':
                case '':
                case 'v': posi+=15*32*bits; break;
                case GIIK_PageUp:
                case ',':
                case '':
                case 'u': if(posi>15*32*bits)posi-=15*32*bits;else posi=0; break;
                case GIIK_Right:
                case '':
                case 'e': posi+=widthharppaus; break;
                case GIIK_Left:
                case '':
                case 'q': if(posi>widthharppaus)posi-=widthharppaus;else posi=0; break;
                case 'p': swap=(swap+1)%4; break;
                case 'b': bits=bits==8?16:bits==16?32:8; break;
                case 'n': nes^=1; break;
                case 'y': LoadPokeFont(fp); /* fall to ^L */
                case '': goto Redraw;
                case '-': if(ylimit>1)--ylimit; break;
                case '+': if(ylimit<128)++ylimit; break;
            }
        } while(ggiKbhit(vis));
    }
}

int main(int argc, const char *const *argv)
{
    if(argc<2 || access(argv[argc-1], O_RDONLY))
    {
        printf(
            "Designed for Game Boy, SNES and NES files, but can be used on others as well\n"
            "Copyright (C) 1992,2002 Bisqwit (http://bisqwit.iki.fi/)\n"
            "Usage: xray <filename>\n");
        return 0;
    }
    Init();
    
    SetPal();
    
    Disp(argv[argc-1]);
    
    Done();

    return 0;
}

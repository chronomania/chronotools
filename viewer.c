#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termio.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <slang.h>

/*
 Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 
 ROM viewer:
 
 You need: SLang (it's a library like ncurses)
 To compile: 
   gcc -Wall -W -pedantic -O2 -o viewer viewer.c -lslang
 To use:
   viewer filename.smc

 * No warranty. You are free to modify this source and to
 * distribute the modified sources, as long as you keep the
 * existing copyright messages intact and as long as you
 * remember to add your own copyright markings.
 * You are not allowed to distribute the program or modified versions
 * of the program without including the source code (or a reference to
 * the publicly available source) and this notice with it.
*/

static size_t size;
static unsigned char *data;

static unsigned char add=0;

static unsigned pos = 0;

static int X=0, Y=0;
static int LINES,COLS;

static struct
{
    unsigned char s[40];
    unsigned len;
    unsigned step;
} Search[2];

enum csettype
{
    normalset,
    pokemonset,
    chronoset,
    
    CSETCOUNT
};

static int breaks=0;
static int fullhex=0;
static enum csettype usecset = normalset;

static char merktab[4][4][4][4];

static void initmerktab(void)
{
    //FILE *fp = fopen("/usr/lib/kbd/consolefonts/cp437-8x8", "rb");
    //FILE *fp = fopen("/home/root/pokedex/cp437-8x8", "rb");
    FILE *fp = fopen("cp437-8x8", "rb");
    unsigned char Fontti[256][8];
    int a,b,c,d;
    static struct {int a,b,c,d; } mrk[256];
    
    if(fp)
    {
        fread(&Fontti, 8, 256, fp);
        fclose(fp);
    }
    
    memset(&merktab, sizeof(merktab), 0);
    
    memset(&mrk, sizeof(mrk), 0);
    
    for(c=0; c<256; c++)
    {
        int y, x;
        
        for(y=0; y<8; y++)
            for(x=0; x<8; x++)
                if(Fontti[c][y] & (1 << (7-x)))
                {
                    if(y<4)if(x<4)mrk[c].a++;else mrk[c].b++;
                    else   if(x<4)mrk[c].c++;else mrk[c].d++;
                }
        
        mrk[c].a = (mrk[c].a+3)*3/16;
        mrk[c].b = (mrk[c].b+3)*3/16;
        mrk[c].c = (mrk[c].c+3)*3/16;
        mrk[c].d = (mrk[c].d+3)*3/16;
        
    }
    
    for(a=0; a<4; a++)
        for(b=0; b<4; b++)
            for(c=0; c<4; c++)
                for(d=0; d<4; d++)
                {
                    int ch, be = INT_MAX, n=0;
                    for(ch=0; ch<255; ch++)
                    {
                        int ad = mrk[ch].a-a;
                        int bd = mrk[ch].b-b;
                        int cd = mrk[ch].c-c;
                        int dd = mrk[ch].d-d;
                        int e = (ad<0?-ad:ad)
                               +(bd<0?-bd:bd)
                               +(cd<0?-cd:cd)
                               +(dd<0?-dd:dd);
                        /* ad*ad+bd*bd+cd*cd+dd*dd; */
                        if(e<be || (e==be && ((ch>=176&&ch<=178)||(ch>=219&&ch<=223)))) n=ch, be=e;
                    }
                    merktab[a][b][c][d] = n;
                }
}

static unsigned char REV(unsigned char n)
{
    n = ((n>>1) & 0x55) | ((n<<1) & 0xAA); // Swap 0[1], 1[1]
    n = ((n>>2) & 0x33) | ((n<<2) & 0xCC); // Swap 0[2], 2[2]
    n = ((n>>4) & 0x0F) | ((n<<4) & 0xF0); // Swap 0[4], 4[4]
    return n;
}


static int Equal(const unsigned char *s, const unsigned char *s2, unsigned step, int len)
{
    for(;;)
    {
        if(!len)return 1;
        if(*s2 != *s)return 0;
        s2 += step;
        ++s;
        --len;
    }
}

static long SlangPos=0;
static int OpenVCSA()
{
    //return open("/dev/vcsa0", O_WRONLY);
    SLsig_block_signals();
    SLsmg_normal_video();
    return 0;
}
static void WriteVCSA(int fd, unsigned chr, unsigned attr)
{
	attr = (unsigned char) attr;
	chr  = (unsigned char) chr;
    //if(chr < 0x20 || chr > 0x7E)chr = '.';
    SLsmg_gotorc(SlangPos/COLS, SlangPos%COLS);
    SLsmg_set_color(attr);
    SLsmg_write_char(chr);
    ++SlangPos;
    fd=fd;
    //const char Buf[2] = {chr,attr};
    //write(fd, Buf, 2);
}
static long TellVCSA(int fd)
{
    //return lseek(fd, 0, SEEK_CUR);
    fd=fd;
    return 4 + SlangPos*2;
}
static void SeekVCSA(int fd, long pos)
{
    //lseek(fd, pos, SEEK_SET);
    fd=fd;
    SlangPos = (pos-4)/2;
}
static void CloseVCSA(int fd)
{
    //close(fd);
    fd=fd;
    SLsmg_refresh();
    SLsig_unblock_signals();
}

static void display(void)
{
    int fd = OpenVCSA();
    int x, y;
    unsigned char *s;
    
    int xe = COLS - 4 * ((COLS-X)/4);
    
    int searchtype=0;
    int searchstep=1;
    
    if(fd < 0)
    {
        perror("/dev/vcsa0");
        exit(-1);
    }
    
    s = data + pos;
    
    for(y=0; y<LINES/4; y++)
        for(x=0; x < (COLS-X)/4; x++)
        {
            char tab[8][8];
            int cy, cx;
            char attr = 0x70;
            for(cy=0; cy<8; cy++)
            {
                unsigned char b1 = *s++;
                unsigned char b2 = *s++;
                for(cx=0; cx<8; cx++)
                    tab[cy][cx] = ((b1 >> (7-cx))&1) | (((b2 >> (7-cx))&1)<<1);
            }
            for(cy=0; cy<4; cy++)
            {
                SeekVCSA(fd, 2 * (2 + COLS*(1+cy+y*4) + xe + x*4));
                for(cx=0; cx<4; cx++)
                {
                    char c  = merktab[tab[cy*2][cx*2]]
                                     [tab[cy*2][cx*2+1]]
                                     [tab[cy*2+1][cx*2]]
                                     [tab[cy*2+1][cx*2+1]];
                    WriteVCSA(fd, c, attr);
                }
            }
        }
    
    s = data + pos;
    
    x=0, y=0;
    
    SeekVCSA(fd, 2 * (COLS+2));
    
    for(;;)
    {
        static const char set1[256] =
           "\r¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶"  // 0x00
            "¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶"  // 0x20
          "'\"gÚÄ¿³³Ã ´Ã´¶\t¶;¶¶¶¶¶¶¶¶¶¶¶¶¶¶."  // 0x40
            "$¶¶IN¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶ "  // 0x60
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ();:[]"  // 0x80
            "abcdefghijklmnopqrstuvwxyz‚'''''"  // 0xA0
            "                                "  // 0xC0
            "'PM-''?!.¶¶¶¶¶¶-$*./,+0123456789"; // 0xE0
        static const char set2[256] = 
            "                                "
            "                                "
            "                ;               "
            "   do                           "
            "                                "
            "                           dlstv"
            "                                "
            " kn rm                          ";

        static const char set3[256] =
            "¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶"   // 00
            "¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶"   // 20
            "¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶"   // 40
            "¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶"   // 60
            "¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶"   // 80
#if 0
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"   // A0
            "ghijklmnopqrstuvwxyz0123456789!?"   // C0
            "/`':&()'.,=-+%¶ ¶¶¶#¶¶¶¶¶¶¶¶¶¶¶_"   // E0
#else
	        "ABCDEFGHIJKLMNOP"   // A0
	        "QRSTUVWXYZabcdef"   // B0
	        "ghijklmnopqrstuv"   // C0
	        "wxyz0123456789ÅÄ"   // D0
	        "Ö«»:-()'.,åäöé¶ "   // E0  EE=musicsymbol
	        "¶¶%É=&+#!?¶¶¶/¶_";  // F0  F0=heartsymbol, F1=..., F2=infinity
#endif
        
        #define writ(byt, attr) do{\
            WriteVCSA(fd, (byt), (attr)); \
            if(++x>=X){SeekVCSA(fd,TellVCSA(fd)+(COLS-x)*2);x=0;if(++y>=Y-3)goto Done;}}while(0) \
            
        switch(searchtype)
        {
            case 0:
            {
                unsigned a, b = (sizeof Search) / (sizeof Search[0]);
                for(a=0; a<b; a++)
                    if(Search[a].len)
                        if(Equal(Search[a].s, s, Search[a].step, Search[a].len))
                        {
                            searchtype = Search[a].len * (searchstep = Search[a].step);
                        }
                break;
            }
            default:
                --searchtype;
        }
        
        if(s >= data+size)
        {
            writ("EOF "[x%4], 0x10);
            continue;
        }
            
        char attr = searchtype?14:7;
        char chr = *s + add;
        switch(usecset)
        {
            case normalset:
                break;
            case pokemonset:
            {
            	unsigned char c = (unsigned char)chr;
                chr  = set1[c];
                break;
            }
            case chronoset:
            {
            	unsigned char c = (unsigned char)chr;
                chr  = set3[c];
                break;
            }
            default:
                chr=0;
        }
        
        if((*s && fullhex) || fullhex==2)goto Hex;
        
        if(usecset == pokemonset && set2[*s] != ' ')
        {
            if(set2[*s] != ';')
            {
                attr = searchtype?10:2;
                writ(chr, attr);
                chr = set2[*s];
            }
            else
            {
                attr = searchtype?15:9;
            }
        }
        
        if(usecset == pokemonset && *s==0x54)
        {
            attr = searchtype?10:2;
            writ('P', attr);
            writ('O', attr);
            writ('K', attr);
            chr = '‚';
        }
        
        if(usecset == chronoset
        && (unsigned char)*s >= 0x21
        && (unsigned char)*s <  0xA0)
        {
        	unsigned dict_ind = (unsigned char)*s - 0x21;
        	unsigned dict_ptr  = 0x200+0x1EFA00 + dict_ind*2;
        	unsigned dict_addr = 0x200+0x1E0000 + data[dict_ptr] + 256*data[dict_ptr+1];
        	unsigned count=data[dict_addr++];
        	
        	/*
        	attr = 12;
        	writ("0123456789ABCDEF"[(dict_addr>>20)&15], attr);
        	writ("0123456789ABCDEF"[(dict_addr>>16)&15], attr);
        	writ("0123456789ABCDEF"[(dict_addr>>12)&15], attr);
        	writ("0123456789ABCDEF"[(dict_addr>> 8)&15], attr);
        	writ("0123456789ABCDEF"[(dict_addr>> 4)&15], attr);
        	writ("0123456789ABCDEF"[(dict_addr    )&15], attr);
        	*/
        	attr = searchtype?10:2;
        	
        	while(count-- > 0)
        	{
        		chr = data[dict_addr++];
        		chr = set3[(unsigned char)chr];
        		if(count) writ(chr, attr);
        	}
        }
        
        if((breaks==1 && !*s) || (breaks==2 && (*s=='\n')))
        {
            if(s[1]!=*s)
            {
                s++;
                writ(chr, attr);
                while(x!=0)writ(' ',attr);
                continue;
            }
        }

        if(usecset != normalset && chr == '¶')
        {
Hex:        attr = (searchtype && !(searchtype%searchstep)) ? 11:3;
            writ(("0123456789ABCDEF"[(unsigned char)*s>>4]), attr);
            chr = "0123456789ABCDEF"[*s&15];
        }

        writ(chr, attr);
        s++;
    }
Done:
    CloseVCSA(fd);
} 

static struct termio back;

static int tctl(int wait)
{
    struct termio term = back;
    term.c_lflag &= ~ECHO & ~ICANON;
    term.c_cc[VMIN] = wait;         /* when set to 0, getch() will return */
    return ioctl(0, TCSETA, &term); /* immediately, if no key was pressed */
                                    /* (return value is then EOF)         */
}

static int LastNap = -1;
static int getch(void)
{
    if(LastNap >= 0)
    {
        int temp = LastNap;
        LastNap = -1;
        return temp;
    }
    tctl(1);
    return getchar();
}
static int kbhit(void) 
{
    int c;
    if(LastNap >= 0)return 1;
    tctl(0);
    c = getchar();
    if(c==EOF)
    {
        LastNap = -1;
        return 0;
    }
    LastNap = c;
    return 1;
}

static void Upd(void)
{
    SLsmg_gotorc(0,0);
    SLsmg_set_color(0x1F);
    SLsmg_printf("Add:%4d Pos:%08X ", add, pos);
    SLsmg_erase_eol();
    
    if(Search[0].len)
    {
        unsigned char *s = Search[0].s;
        unsigned l = Search[0].len;
        SLsmg_printf(" (Searched:");
        for(;l;--l)SLsmg_printf(" %02X", *s++);
        SLsmg_printf(")");
    }
    else
    {
        SLsmg_printf("      Usage: viewer <filename>");
    }
    
    SLsmg_gotorc(0, COLS-13);
    SLsmg_printf("B%d H%d P%d X%d", breaks,fullhex,usecset, X);
    
    SLsmg_refresh();
}

static void DoSearch(unsigned step, const unsigned char *what, int len)
{
    static size_t oldfound;
    size_t p=0, plen = size-len-1;
    if(!len)
    {
        len = Search[0].len;
        what = Search[0].s;
        step = Search[0].step;
        p = oldfound+1;
    }
    else
    {
        memmove(&Search[1], &Search[0], sizeof(Search) - sizeof(Search[0]));
        memcpy(Search[0].s, what, Search[0].len=len);
        Search[0].step = step;
    }
    for(; p<plen; p++)
    {
        if(Equal(what, data+p, step, len))
        {
            oldfound = p;
            /* bingo */
            pos = p>X*Y/3 ? p-X*Y/3 : 0;
            return;
        }
    }    
}

static void SlangUpdate(int sig)
{
    static const char *const colours[16] =
    {
        "black","blue","green","cyan","red","magenta","brown","lightgray",
        "gray","brightblue","brightgreen","brightcyan",                   
        "brightred","brightmagenta","yellow","white"   
    };
    unsigned b, f;
    
    sig=sig;
    
    for(b=0; b<16; ++b)
        for(f=0; f<16; ++f)
            SLtt_set_color(b*16+f, NULL,
                (char *)(colours[f]),
                (char *)(colours[b]));
                   
    SLsmg_init_smg();
    SLtt_get_screen_size();
    LINES = SLtt_Screen_Rows;
    COLS = SLtt_Screen_Cols;
    SLang_init_tty(0,0,0);
    SLsmg_cls();
    X=COLS, Y=LINES;
}
static void SlangInit()
{
    SLang_init_tty(0,0,0);
    
    SLsig_block_signals();
    SLtt_get_terminfo();  
    /* SLkp_init assumes that SLtt_get_terminfo has been called. */
    if(SLkp_init() < 0)                                            
    {                  
        SLsig_unblock_signals();
        return;
    }
    SLsig_unblock_signals();
    signal(SIGWINCH, SlangUpdate);
    SlangUpdate(0);
}

int main(int argc, const char *const *argv)
{
    FILE *fp;
    const char *fn = argc>1?argv[argc-1]:NULL;
    
    initmerktab();
    
    ioctl(0, TCGETA, &back); tctl(1);
    
    SlangInit();
    
Restart:
    fp = fn?fopen(fn, "rb"):NULL;
    if(!fp)
    {
        if(fn)
        {
            SLsmg_gotorc(LINES-1,0);
            SLsmg_set_color(7);
            SLsmg_erase_eol();
            SLsmg_printf("\nCan't open %s: %s\n", fn, strerror(errno));
        }
        SLsmg_printf
        (
            "\n\n"
            "A file viewer, designed specially for investigating Game Boy Pokémon-games.\n"
            "Copyright (C) 1992,2002 Bisqwit (http://iki.fi/bisqwit/)\n"
            "Usage: viewer <filename>\n\n"
        );
        goto RealEnd;
    }
    
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    data = (unsigned char *)malloc(size);
    fread(data, size, 1, fp);
    
    fclose(fp);
    
    Upd();
    
    SLsmg_gotorc(LINES-2, 0);
    SLsmg_set_color(0x30);
    SLsmg_erase_eol();
    SLsmg_printf("q=quit   .=pgdn ,=pgup   a,s=-+wide   /=find    f=file\n");
    SLsmg_erase_eol();
    SLsmg_printf("+-=add b=toglbreaks h=toglhex p=toglpokemontrans");

    for(;;)
    {
        if(!kbhit())display();
        switch(getch())
        {
            case 'q': goto End;
            case '-': add--; Upd(); break;
            case '+': add++; Upd(); break;
            case '': Lt: if(pos>0)pos--; Upd(); break;
            case '': Rt: pos++; Upd(); break;
            case '': Dn: if(fullhex)pos+=X/2;else pos+=X; Upd(); break;
            case '': Up: if(pos>X)pos-=X;else pos=0; Upd(); break;
            case '': case '.': case ' ': pos += X*Y/2; Upd(); break;
            case '': case ',': if(pos>X*Y)pos -= X*Y/2;else pos=0; Upd(); break;
            case 'a': if(X>=4)--X; Upd(); break;
            case 's': if(X<900)++X; Upd(); break;
            case 'b': breaks=((breaks+1)%3); Upd(); break;
            case 'h': fullhex=((fullhex+1)%3); Upd(); break;
            case 'p': usecset = (usecset+1)%CSETCOUNT; Upd(); break;
            case '<': if(pos)pos = (pos-1) & ~ 0x3FFF; Upd(); break;
            case '>': pos = (pos+0x4000) & ~0x3FFF; Upd(); break;
            case 'z': if(pos>=0x4000)pos-=0x4000; Upd(); break;
            case 'Z': pos+=0x4000; Upd(); break;
            case 'G':
            {
                unsigned char bank = data[pos];
                if(bank==19)bank=31;
                else if(bank==20)bank=32;
                else if(bank==31)bank=46;
                pos = (bank-1)*0x4000 + data[pos+2]*256 + data[pos+1];
                Upd();
                break;
            }
            case '\33':
            {
                int c, n = 0;
                for(;;)
                {
                    c = getch();
                    if(c=='A')goto Up;
                    if(c=='B')goto Dn;
                    if(c=='C')goto Rt;
                    if(c=='D')goto Lt;
                    if(c>='0'&&c<='9'){n=n*10+c-'0';continue;}
                    if(c!=';'&&c!='[')break;
                }
                break;
            }
            case 'f':
            {
                char Buf[256];
                SLsmg_gotorc(0,0);
                SLsmg_set_color(7);
                SLsmg_printf("Load file: ");
                SLsmg_set_color(0x17);
                SLsmg_erase_eol();
                SLsmg_refresh();
                
                ioctl(0, TCSETA, &back);
                fgets(Buf, 255, stdin);
                if(Buf[0]!='\n')
                {
                    strtok(Buf, "\n");
                    if(access(Buf, R_OK))
                    {
                        SLsmg_gotorc(0,0);
                        SLsmg_set_color(0x4F);
                        SLsmg_printf("Error: %s - press key\n", strerror(errno));
                        getch();
                    }
                    else
                    {
                        free(data);
                        fn = strdup(Buf);
                        /* memory leak, don't care... who is going */
                        /* to load 100000 times a different file anyway. */
                        goto Restart;
                    }
                }
                break;
            }
            case '/':
            {
                char Buf[512], *t;
                const char *s;
                unsigned step=1;
                unsigned base = 16;
                SLsmg_gotorc(0,22);
                SLsmg_set_color(0x13);
                SLsmg_printf("Search ([:step:][d]num,num,..|\"string): ");
                SLsmg_set_color(7);
                SLsmg_erase_eol();
                SLsmg_refresh();
                
                ioctl(0, TCSETA, &back);
                
                fgets(Buf,sizeof Buf,stdin);
                
                s=Buf;
                if(Buf[0]=='"')
                {
                    ++s;
                    for(t=Buf; *s&&*s!='\n'; ++s)
                    {
                        unsigned char c = (unsigned char)(*s + 256 - add);
                        if(usecset == chronoset)
                        {
                            if(c >= 'A' && c <= 'Z')c = c-'A'+0xA0;
                            else if(c >= 'a' && c <= 'z')c = c-'a'+0xBA;
                            else if(c >= '0' && c <= '9')c = c-'0'+0xD4;
                        }
                        *t++ = c;
                    }
                }
                else
                {
                    if(Buf[0]==':')
                    {
                        step = strtol(Buf+1, &s, 10);
                        if(*s==':')++s;
                    }
                    if(*s=='d')++s,base=10;
                    for(t=Buf; *s&&*s!='\n'; s++)
                    {
                        int i = strtol(s, &s, base);
                        *t++ = i;
                        if(*s!=',')break;
                    }
                }
                DoSearch(step, Buf, t-Buf);
                
                Upd();
                tctl(1);
                break;
            }
        }
    }
End:
    SLsmg_gotorc(LINES-1,0);
    SLsmg_set_color(7);
    SLsmg_erase_eol();
    SLsmg_printf("\n");
RealEnd:    
    ioctl(0, TCSETA, &back);
    return 0;    
}

#include "o65.hh"

static void DisAsm(unsigned origin, const unsigned char *data, unsigned length);

int main(void)
{
    O65 tmp;
    
    FILE *fp = fopen("testi.o65", "rb");
    tmp.Load(fp);
    fclose(fp);
    
    printf("Code size: %u bytes\n", tmp.GetCodeSize());
    
    printf(" TextDisplay is at %06X\n", tmp.GetSymAddress("TextDisplay"));
    
    unsigned origin = 0xC12440;
    
    printf("After relocating code at %06X:\n", origin);
    
    tmp.LocateCode(origin);
    
    printf(" TextDisplay is at %06X\n", tmp.GetSymAddress("TextDisplay"));
    
    printf("Linking external symbols: MSG at PAGE=$FF, OFFS=$9000, LENGTH=10\n");
    tmp.LinkSym("MSG_PAGE",   0xFF);
    tmp.LinkSym("MSG_OFFS",   0x9000);
    tmp.LinkSym("MSG_LENGTH", 10);
    tmp.LinkSym("LONGSYM",    0x123456);
    
    printf("Verifying that everything has been done\n");
    
    tmp.Verify();
    
    const vector<unsigned char>& code = tmp.GetCode();
    printf("Code:\n");
    DisAsm(origin, &code[0], code.size());
    return 0;
}


/* Bisqwit's humble little snes-disassembler. */
static void DisAsm(unsigned origin, const unsigned char *data, unsigned length)
{
    bool A=true,X=true;
    for(unsigned size,address=origin; length>0;
        address+=size,length-=size,data+=size)
    {
        char c, Buf[64], *info="-------xy-xy-y-xy-x-y--x---###-----((([[-----"
 "-(([(---ax1rR111111112223311222-m---------)))]]-----sS)])--1--2232222222233"
 "3442233313DKDTGGGMABYAOOORELJUGHHNAQYAOPPSOKRTGGGMABYAOOORELJUHHHNAQYAPPPSA"
 "KDTZGGMABYAOOORELJUZHHNAQAARPPSAKFTGGGMABYAVOORELJUHHHNAQAAXPPSEKFTGGGMABAA"
 "OOORELJUHHINAQAAOPPSCKCTGGGMABAAOOORELJUHHINAQAAPPQSCKDTGGGMABAAOOORELJJBHH"
 "NAQAAWPPSCKDTGGGMABAAOOORELJUOHHNAQAAXPPSADCANDASLBCCBCSBEQBITBMIBNEBPLBRAB"
 "RKBRLBVCBVSCLCCLDCLICLVCMPCOPCPXCPYDB DECDEXDEYEORINCINXINYJMLJMPJSLJSRLDAL"
 "DXLDYLSRMVNMVPNOPORAPEAPEIPERPHAPHBPHDPHKPHPPHXPHYPLAPLBPLDPLPPLXPLYREPROLR"
 "ORRTIRTLRTSSBCSECSEDSEISEPSTASTPSTXSTYSTZTAXTAYTCDTCSTDCTRBTSBTSCTSXTXATXST"
 "XYTYATYXWAIXBAXCE.M7MtM%MUM%StM%M,MMMsM%M2M?qsM%ME$D$)$_$[$_Z)$_$*$$$)$_$e$"
 ";u)$_$a>:>K>I>Q>ITC>I>0>>>J>I>4>WpC>I>c#P#m#`#X#`bC#`#1###m#`#g#]rC#`#-i/il"
 "iki=)wRliki&iiilikizixymimiHFGFHFGFoFnYHFGF'FFFHFGF5Fv{HFGF96^696;6A6<|96;6"
 "+666O6;636VjB6;68dhd8d?d@dL}8d?d(dddNd?dfd\\~Ed?d",
          *s = Buf + sprintf(Buf, "%.3s ", info+281 + 3*info[*data+662]);
        if(*data==0xE2){c=data[1];if(c&0x20)A=false; if(c&0x10)X=false; }
        if(*data==0xC2){c=data[1];if(c&0x20)A=true;  if(c&0x10)X=true; }
        size=info[*data+130]-'A';
        if('-'!=(c=info[size+26]))*s++=c;
        if('-'!=(c=info[size+52]))
        {
            if(c=='a')c='1'+A;if(c=='x')c='1'+X;
            if(c=='m')s+=sprintf(s,"$%02X $%02X",data[1],data[2]);
            else if(c=='r'){ signed char n=data[1];
                             s+=sprintf(s,"%+d (%06X)",n,address+n+2); }
            else if(c=='R'){ signed short n=data[1]+data[2]*256;
                             s+=sprintf(s,"%+d (%06X)",n,address+n+3); }
            else{*s++='$';for(c-='0';c;s+=sprintf(s,"%02X",data[c--]));}
        }
        if(info[size]=='x')s+=sprintf(s,",x");
        if('-'!=(c=info[size+78]))
            if(c=='s')s+=sprintf(s,",s");
            else if(c=='S')s+=sprintf(s,",s)");
            else *s++=c;
        if(info[size]=='y')s+=sprintf(s,",y");
        if(size==1)size=2+A;else if(size==2)size=2+X;
        else size=info[size+104]-'0';
        printf(" %06X\t", address);
        for(unsigned n=0; n<4; ++n)
            printf(n<size?"%02X ":"   ", data[n]);
        
        *s=0; printf("%s\n", Buf);
        if(length < size)break;
    }
}

#include "insdata.hh"
#include "assemble.hh"

const struct AddrMode AddrModes[] =
{
    /* Sorted in priority order - but the no-parameters-type must come first! */

  /* num  example      reject prereq postreq p1-type         p2-type */
  { /* 0  nop          */   0, "",   "",   AddrMode::tNone, AddrMode::tNone },//o
  { /* 1  lda #value   */   0, "#",  "",   AddrMode::tA,    AddrMode::tNone },//o #A
  { /* 2  ldx #value   */   0, "#",  "",   AddrMode::tX,    AddrMode::tNone },//o #X
  { /* 3  rep #imm8    */   0, "#",  "",   AddrMode::tByte, AddrMode::tNone },//o #B
  { /* 4  bcc rel8     */   0, "",   "",   AddrMode::tRel8, AddrMode::tNone },//o r
  { /* 5  brl rel16    */   0, "",   "",   AddrMode::tRel16,AddrMode::tNone },//o R
  { /* 6  lda $10      */ '(', "",   "",   AddrMode::tByte, AddrMode::tNone },//o B
  { /* 7  lda $10,x    */ '(', "",   ",x", AddrMode::tByte, AddrMode::tNone },//o B,x
  { /* 8  lda $10,y    */ '(', "",   ",y", AddrMode::tByte, AddrMode::tNone },//o B,y
  { /* 9  lda ($10)    */   0, "(",  ")",  AddrMode::tByte, AddrMode::tNone },//o (B)
  { /* 10 lda ($10,x)  */   0, "(", ",x)", AddrMode::tByte, AddrMode::tNone },//o (B,x)
  { /* 11 lda ($10),y  */   0, "(", "),y", AddrMode::tByte, AddrMode::tNone },//o (B),y
  { /* 12 lda [$10]    */   0, "[",  "]",  AddrMode::tByte, AddrMode::tNone },//o [B] 
  { /* 13 lda [$10],y  */   0, "[", "],y", AddrMode::tByte, AddrMode::tNone },//o [B],y
  { /* 14 lda $1234    */ '(', "",   "",   AddrMode::tWord, AddrMode::tNone },//o W
  { /* 15 lda $1234,x  */ '(', "",   ",x", AddrMode::tWord, AddrMode::tNone },//o W,x
  { /* 16 lda $1234,y  */ '(', "",   ",y", AddrMode::tWord, AddrMode::tNone },//o W,y
  { /* 17 lda $123456  */ '(', "",   "",   AddrMode::tLong, AddrMode::tNone },//o L
  { /* 18 lda $123456,x*/ '(', "",   ",x", AddrMode::tLong, AddrMode::tNone },//o L,x
  { /* 19 lda $10,s    */   0, "",   ",s", AddrMode::tByte, AddrMode::tNone },//o B,s
  { /* 20 lda ($10,s),y*/   0, "(",",s),y",AddrMode::tByte, AddrMode::tNone },//o (B,s),y
  { /* 21 lda ($1234)  */   0, "(", ")",   AddrMode::tWord, AddrMode::tNone },//o (W)
  { /* 22 lda [$1234]  */   0, "[", "]",   AddrMode::tWord, AddrMode::tNone },//o [W]
  { /* 23 lda ($1234,x)*/   0, "(", ",x)", AddrMode::tWord, AddrMode::tNone },//o (W,x)
  { /* 24 mvn $7E,$7F  */   0, "",   "",   AddrMode::tByte, AddrMode::tByte },//o B,B
  { /* 25 pea #imm16   */   0, "#",  "",   AddrMode::tWord, AddrMode::tNone },//o #W
  { /* 26 .link group 1  */ 0, "group", "",AddrMode::tWord, AddrMode::tNone },
  { /* 27 .link page $FF */ 0, "page",  "",AddrMode::tByte, AddrMode::tNone }
};
const unsigned AddrModeCount = sizeof(AddrModes) / sizeof(AddrModes[0]);

const struct ins ins[] =
{
    // IMPORTANT: Alphabetical sorting!

  { ".(",    "sb" }, // start block, no params
  { ".)",    "eb" }, // end block, no params
  { ".al",   "al" }, // A=16bit
  { ".as",   "as" }, // A=8bit
  { ".bss",  "gb" }, // Select seG BSS
  { ".data", "gd" }, // Select seG DATA
  { ".link",         // Select linkage (modes 26 and 27)
           "--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'li'li" },
  { ".text", "gt" }, // Select seG TEXT
  { ".xl",   "xl" }, // X=16bit
  { ".xs",   "xs" }, // X=8bit
  { ".zero", "gz" }, // Select seG ZERO
  
  // ins     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 
  { "adc", "--'69'--'--'--'--'65'75'--'72'61'71'67'77'6D'7D'79'6F'7F'63'73'--'--'--'--'--"},
  { "and", "--'29'--'--'--'--'25'35'--'32'21'31'27'37'2D'3D'39'2F'3F'23'33'--'--'--'--'--"},
  { "asl", "0A'--'--'--'--'--'06'16'--'--'--'--'--'--'0E'1E'--'--'--'--'--'--'--'--'--'--"},
  { "bcc", "--'--'--'--'90'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bcs", "--'--'--'--'B0'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "beq", "--'--'--'--'F0'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bit", "--'89'--'--'--'--'24'34'--'--'--'--'--'--'2C'3C'--'--'--'--'--'--'--'--'--'--"},
  { "bmi", "--'--'--'--'30'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bne", "--'--'--'--'D0'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bpl", "--'--'--'--'10'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bra", "--'--'--'--'80'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "brk", "--'--'--'00'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "brl", "--'--'--'--'--'82'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bvc", "--'--'--'--'50'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bvs", "--'--'--'--'70'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "clc", "18'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "cld", "D8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "cli", "58'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "clv", "B8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "cmp", "--'C9'--'--'--'--'C5'D5'--'D3'C1'D1'C7'D7'CD'DD'D9'CF'DF'C3'--'--'--'--'--'--"},
  { "cop", "--'--'--'02'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "cpx", "--'--'E0'--'--'--'E4'--'--'--'--'--'--'--'EC'--'--'--'--'--'--'--'--'--'--'--"},
  { "cpy", "--'--'C0'--'--'--'C4'--'--'--'--'--'--'--'CC'--'--'--'--'--'--'--'--'--'--'--"},
  { "db",  "--'--'--'42'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "dec", "3A'--'--'--'--'--'C6'D6'--'--'--'--'--'--'CE'DE'--'--'--'--'--'--'--'--'--'--"},
  { "dex", "CA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "dey", "88'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "eor", "--'49'--'--'--'--'45'55'--'52'41'51'47'57'4D'5D'59'4F'5F'43'53'--'--'--'--'--"},
  { "inc", "1A'--'--'--'--'--'E6'F6'--'--'--'--'--'--'EE'FE'--'--'--'--'--'--'--'--'--'--"},
  { "inx", "E8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "iny", "C8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "jml", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'DC'--'--'--"},
  { "jmp", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'4C'--'--'5C'--'--'--'6C'--'7C'--'--"},
  { "jsl", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'22'--'--'--'--'--'--'--'--"},
  { "jsr", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'20'--'--'--'--'--'--'--'--'FC'--'--"},
  { "lda", "--'A9'--'--'--'--'A5'B5'--'B2'A1'B1'A7'B7'AD'BD'B9'AF'BF'A3'B3'--'--'--'--'--"},
  { "ldx", "--'--'A2'--'--'--'A6'--'B6'--'--'--'--'--'AE'--'BE'--'--'--'--'--'--'--'--'--"},
  { "ldy", "--'--'A0'--'--'--'A4'B4'--'--'--'--'--'--'AC'BC'--'--'--'--'--'--'--'--'--'--"},
  { "lsr", "4A'--'--'--'--'--'46'56'--'--'--'--'--'--'4E'5E'--'--'--'--'--'--'--'--'--'--"},
           // For convenience, MVN and MVP allow $aa,$bb or $bbaa
  { "mvn", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'54'--'--'--'--'--'--'--'--'--'54'--"},
  { "mvp", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'44'--'--'--'--'--'--'--'--'--'44'--"},
  { "nop", "EA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "ora", "--'09'--'--'--'--'05'15'--'12'01'11'07'17'0D'1D'19'0F'1F'03'13'--'--'--'--'--"},
           // For convenience, PEA allows $1234 or #$1234
  { "pea", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'F4'--'--'--'--'--'--'--'--'--'--'F4"},
           // For convenience, PEI allows $12 or ($12).
  { "pei", "--'--'--'--'--'--'D4'--'--'D4'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "per", "--'--'--'--'--'62'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "pha", "48'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phb", "8B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phd", "0B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phk", "4B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "php", "08'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phx", "DA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phy", "5A'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "pla", "68'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "plb", "AB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "pld", "2B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "plp", "28'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "plx", "FA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "ply", "7A'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "rep", "--'--'--'C2'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "rol", "2A'--'--'--'--'--'26'36'--'--'--'--'--'--'2E'3E'--'--'--'--'--'--'--'--'--'--"},
  { "ror", "6A'--'--'--'--'--'66'76'--'--'--'--'--'--'6E'7E'--'--'--'--'--'--'--'--'--'--"},
  { "rti", "40'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "rtl", "6B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "rts", "60'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sbc", "--'E9'--'--'--'--'E5'F5'--'F2'E1'F1'E7'F7'ED'FD'F9'EF'FF'E3'F3'--'--'--'--'--"},
  { "sec", "38'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sed", "F8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sei", "78'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sep", "--'--'--'E2'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sta", "--'--'--'--'--'--'85'95'--'92'81'91'87'97'8D'9D'99'8F'9F'83'93'--'--'--'--'--"},
  { "stp", "DB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "stx", "--'--'--'--'--'--'86'--'96'--'--'--'--'--'8E'--'--'--'--'--'--'--'--'--'--'--"},
  { "sty", "--'--'--'--'--'--'84'94'--'--'--'--'--'--'8C'--'--'--'--'--'--'--'--'--'--'--"},
  { "stz", "--'--'--'--'--'--'64'74'--'--'--'--'--'--'9C'9E'--'--'--'--'--'--'--'--'--'--"},
  { "tax", "AA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tay", "A8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tcd", "5B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tcs", "1B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tdc", "7B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "trb", "--'--'--'--'--'--'14'--'--'--'--'--'--'--'1C'--'--'--'--'--'--'--'--'--'--'--"},
  { "tsb", "--'--'--'--'--'--'04'--'--'--'--'--'--'--'0C'--'--'--'--'--'--'--'--'--'--'--"},
  { "tsc", "3B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tsx", "BA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "txa", "8A'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "txs", "9A'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "txy", "9B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tya", "98'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tyx", "BB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "wai", "CB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "xba", "EB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "xce", "FB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"}
};
const unsigned InsCount = sizeof(ins) / sizeof(ins[0]);

unsigned GetOperand1Size(unsigned modenum)
{
    if(modenum < AddrModeCount)
        switch(AddrModes[modenum].p1)
        {
            case AddrMode::tRel8:
            case AddrMode::tByte: return 1;
            case AddrMode::tRel16:
            case AddrMode::tWord: return 2;
            case AddrMode::tLong: return 3;
            case AddrMode::tA: return A_16bit ? 2 : 1;
            case AddrMode::tX: return X_16bit ? 2 : 1;
            case AddrMode::tNone: ;
        }
    return 0;
}

unsigned GetOperand2Size(unsigned modenum)
{
    if(modenum < AddrModeCount)
        switch(AddrModes[modenum].p2)
        {
            case AddrMode::tRel8:
            case AddrMode::tByte: return 1;
            case AddrMode::tRel16:
            case AddrMode::tWord: return 2;
            case AddrMode::tLong: return 3;
            case AddrMode::tA: return A_16bit ? 2 : 1;
            case AddrMode::tX: return X_16bit ? 2 : 1;
            case AddrMode::tNone: ;
        }
    return 0;
}

unsigned GetOperandSize(unsigned modenum)
{
    return GetOperand1Size(modenum) + GetOperand2Size(modenum);
}

bool IsReservedWord(const std::string& s)
{
    const struct ins *insdata = std::lower_bound(ins, ins+InsCount, s);
    return (insdata < ins+InsCount && s == insdata->token)
         || s == ".byt"
         || s == ".word";
}

#include <cctype>
#include <string>
#include <utility>
#include <vector>
#include <map>

#undef EOF

#include "tristate.hh"

static const char FORCE_LOBYTE = '<';
static const char FORCE_HIBYTE = '>';
static const char FORCE_ABSWORD= '!';
static const char FORCE_LONG   = '@';
static const char FORCE_SEGBYTE= '^';
static const char FORCE_REL8   = 1;
static const char FORCE_REL16  = 2;

static bool A_16bit = true;
static bool X_16bit = true;

const struct AddrMode
{
    char forbid;
    char prereq[2];
    char postreq[6];
    enum { tNone, tByte, tWord, tLong, tA, tX, tRel8, tRel16 } p1, p2;
} AddrModes[] =
{
    /* Sorted in priority order - but NOP type must come first! */

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
  { /* 24 mvn $7E,$7F  */   0, "",   "",   AddrMode::tByte, AddrMode::tByte } //o B,B
};
const unsigned AddrModeCount = sizeof(AddrModes) / sizeof(AddrModes[0]);

const struct ins
{
    char token[4];
    char opcodes[25*3];

    bool operator< (const std::string &s) const { return s > token; }
} ins[] =
{
  { ".(",  "sb"}, // start block, no params
  { ".)",  "eb"}, // end block, no params
  { ".al", "al"}, // A=16bit
  { ".as", "as"}, // A=8bit
  { ".xl", "xl"}, // X=16bit
  { ".xs", "xs"}, // X=8bit
  
  // ins     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
  { "adc", "--'69'--'--'--'--'65'75'--'72'61'71'67'77'6D'7D'79'6F'7F'63'73'--'--'--'--"},
  { "and", "--'29'--'--'--'--'25'35'--'32'21'31'27'37'2D'3D'39'2F'3F'23'33'--'--'--'--"},
  { "asl", "0A'--'--'--'--'--'06'16'--'--'--'--'--'--'0E'1E'--'--'--'--'--'--'--'--'--"},
  { "bcc", "--'--'--'--'90'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bcs", "--'--'--'--'B0'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "beq", "--'--'--'--'F0'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bit", "--'89'--'--'--'--'24'34'--'--'--'--'--'--'2C'3C'--'--'--'--'--'--'--'--'--"},
  { "bmi", "--'--'--'--'30'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bne", "--'--'--'--'D0'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bpl", "--'--'--'--'10'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bra", "--'--'--'--'80'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "brk", "--'--'--'00'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "brl", "--'--'--'--'--'82'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bvc", "--'--'--'--'50'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "bvs", "--'--'--'--'70'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "clc", "18'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "cld", "D8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "cli", "58'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "clv", "B8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "cmp", "--'C9'--'--'--'--'C5'D5'--'D3'C1'D1'C7'D7'CD'DD'D9'CF'DF'C3'--'--'--'--'--"},
  { "cop", "--'--'--'02'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "cpx", "--'--'E0'--'--'--'E4'--'--'--'--'--'--'--'EC'--'--'--'--'--'--'--'--'--'--"},
  { "cpy", "--'--'C0'--'--'--'C4'--'--'--'--'--'--'--'CC'--'--'--'--'--'--'--'--'--'--"},
  { "db",  "--'--'--'42'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "dec", "3A'--'--'--'--'--'C6'D6'--'--'--'--'--'--'CE'DE'--'--'--'--'--'--'--'--'--"},
  { "dex", "CA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "dey", "88'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "eor", "--'49'--'--'--'--'45'55'--'52'41'51'47'57'4D'5D'59'4F'5F'43'53'--'--'--'--"},
  { "inc", "1A'--'--'--'--'--'E6'F6'--'--'--'--'--'--'EE'FE'--'--'--'--'--'--'--'--'--"},
  { "inx", "E8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "iny", "C8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "jml", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'DC'--'--"},
  { "jmp", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'4C'--'--'5C'--'--'--'6C'--'7C'--"},
  { "jsl", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'22'--'--'--'--'--'--'--"},
  { "jsr", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'20'--'--'--'--'--'--'--'--'FC'--"},
  { "lda", "--'A9'--'--'--'--'A5'B5'--'B2'A1'B1'A7'B7'AD'BD'B9'AF'BF'A3'B3'--'--'--'--"},
  { "ldx", "--'--'A2'--'--'--'A6'--'B6'--'--'--'--'--'AE'--'BE'--'--'--'--'--'--'--'--"},
  { "ldy", "--'--'A0'--'--'--'A4'B4'--'--'--'--'--'--'AC'BC'--'--'--'--'--'--'--'--'--"},
  { "lsr", "4A'--'--'--'--'--'46'56'--'--'--'--'--'--'4E'5E'--'--'--'--'--'--'--'--'--"},
  { "mvn", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'54"},
  { "mvp", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'44"},
  { "nop", "EA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "ora", "--'09'--'--'--'--'05'15'--'12'01'11'07'17'0D'1D'19'0F'1F'03'13'--'--'--'--"},
  { "pea", "--'--'--'--'--'--'--'--'--'--'--'--'--'--'F4'--'--'--'--'--'--'--'--'--'--"},
  { "pei", "--'D4'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "per", "--'--'--'--'--'62'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "pha", "48'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phb", "8B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phd", "0B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phk", "4B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "php", "08'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phx", "DA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "phy", "5A'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "pla", "68'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "plb", "AB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "pld", "2B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "plp", "28'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "plx", "FA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "ply", "7A'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "rep", "--'--'--'C2'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "rol", "2A'--'--'--'--'--'26'36'--'--'--'--'--'--'2E'3E'--'--'--'--'--'--'--'--'--"},
  { "ror", "6A'--'--'--'--'--'66'76'--'--'--'--'--'--'6E'7E'--'--'--'--'--'--'--'--'--"},
  { "rti", "40'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "rtl", "6B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "rts", "60'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sbc", "--'E9'--'--'--'--'E5'F5'--'F2'E1'F1'E7'F7'ED'FD'F9'EF'FF'E3'F3'--'--'--'--"},
  { "sec", "38'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sed", "F8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sei", "78'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sep", "--'--'--'E2'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "sta", "--'--'--'--'--'--'85'95'--'92'81'91'87'97'8D'9D'99'8F'9F'83'93'--'--'--'--"},
  { "stp", "DB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "stx", "--'--'--'--'--'--'86'--'96'--'--'--'--'--'8E'--'--'--'--'--'--'--'--'--'--"},
  { "sty", "--'--'--'--'--'--'84'94'--'--'--'--'--'--'8C'--'--'--'--'--'--'--'--'--'--"},
  { "stz", "--'--'--'--'--'--'64'74'--'--'--'--'--'--'9C'9E'--'--'--'--'--'--'--'--'--"},
  { "tax", "AA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tay", "A8'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tcd", "5B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tcs", "1B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tdc", "7B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "trb", "--'--'--'--'--'--'14'--'--'--'--'--'--'--'1C'--'--'--'--'--'--'--'--'--'--"},
  { "tsb", "--'--'--'--'--'--'04'--'--'--'--'--'--'--'0C'--'--'--'--'--'--'--'--'--'--"},
  { "tsc", "3B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tsx", "BA'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "txa", "8A'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "txs", "9A'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "txy", "9B'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tya", "98'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "tyx", "BB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "wai", "CB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "xba", "EB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
  { "xce", "FB'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--'--"},
};
const unsigned InsCount = sizeof(ins) / sizeof(ins[0]);

struct ParseData
{
private:
    std::string data;
    unsigned pos;
public:
    typedef unsigned StateType;
    
    ParseData() : pos(0) { }
    ParseData(const std::string& s) : data(s), pos(0) { }
    ParseData(const char *s) : data(s), pos(0) { }
    
    bool EOF() const { return pos >= data.size(); }
    void SkipSpace() { while(!EOF() && (data[pos] == ' ' || data[pos] == '\t'))++pos; }
    StateType SaveState() const { return pos; }
    void LoadState(const StateType state) { pos = state; }
    char GetC() { return EOF() ? 0 : data[pos++]; }
    char PeekC() const { return EOF() ? 0 : data[pos]; }
    
    const std::string GetRest() const { return data.substr(pos); }
};

const std::string ParseToken(ParseData& data)
{
    std::string token;
    data.SkipSpace();
    char c = data.PeekC();
    if(c == '$')
    {
    Hex:
        token += data.GetC();
        for(;;)
        {
            c = data.PeekC();
            if( (c >= '0' && c <= '9')
             || (c >= 'A' && c <= 'F')
             || (c >= 'a' && c <= 'f')
             || (c == '-' && token.size()==1)
              )
                token += data.GetC();
            else
                break;
        }
    }
    else if(c == '-' || (c >= '0' && c <= '9'))
    {
        if(c == '-')
        {
            ParseData::StateType state = data.SaveState();
            data.GetC();
            char c = data.PeekC();
            data.LoadState(state);
            if(c == '$')
            {
                token += c;
                goto Hex;
            }
            bool ok = c >= '0' && c <= '9';
            if(!ok) return token;
        }
        for(;;)
        {
            token += data.GetC();
            if(data.EOF()) break;
            c = data.PeekC();
            if(c < '0' || c > '9') break;
        }
    }
    if(isalpha(c) || c == '_')
    {
        for(;;)
        {
            token += data.GetC();
            if(data.EOF()) break;
            c = data.PeekC();
            if(c != '_' && !isalnum(c)) break;
        }
    }
    return token;
}

void ReplaceLabelWithValue(const std::string&s, long value, class expression*& e);

class expression
{
public:
    virtual ~expression() { }
    virtual bool IsConst() const = 0;
    virtual long GetConst() const { return 0; }
    
    virtual const std::string Dump() const = 0;
    
    virtual void DefineLabel(const std::string&s, long value) { }
};
class expr_number: public expression
{
protected:
    long value;
public:
    expr_number(long v): value(v) { }
    
    virtual bool IsConst() const { return true; }
    virtual long GetConst() const { return value; }

    virtual const std::string Dump() const
    {
        char Buf[512];
        if(value < 0)
            std::sprintf(Buf, "$-%lX", -value);
        else
            std::sprintf(Buf, "$%lX", value);
        return Buf;
    }
};
class expr_label: public expression
{
protected:
    std::string name;
public:
    expr_label(const std::string& s): name(s) { }
    
    virtual bool IsConst() const { return false; }

    virtual const std::string Dump() const { return name; }
    const std::string& GetName() const { return name; }
};
class expr_unary: public expression
{
public:
    expression* sub;
public:
    expr_unary(expression *s): sub(s) { }
    virtual bool IsConst() const { return sub->IsConst(); }

    virtual ~expr_unary() { delete sub; }
    
    virtual void DefineLabel(const std::string&s, long value)
    {
        ::ReplaceLabelWithValue(s, value, sub);
    }
private:
    expr_unary(const expr_unary &b);
    void operator= (const expr_unary &b);
};
class expr_binary: public expression
{
public:
    expression* left;
    expression* right;
public:
    expr_binary(expression *l, expression *r): left(l), right(r) { }
    virtual bool IsConst() const { return left->IsConst() && right->IsConst(); }

    virtual ~expr_binary() { delete left; delete right; }
    
    virtual void DefineLabel(const std::string&s, long value)
    {
        ::ReplaceLabelWithValue(s, value, left);
        ::ReplaceLabelWithValue(s, value, right);
    }
    
private:
    expr_binary(const expr_binary &b);
    void operator= (const expr_binary &b);
};

#define unary_class(classname, op, stringop) \
    class classname: public expr_unary \
    { \
    public: \
        classname(expression *s): expr_unary(s) { } \
        virtual long GetConst() const { return op sub->GetConst(); } \
    \
        virtual const std::string Dump() const \
        { return std::string(stringop) + sub->Dump(); } \
    };

#define binary_class(classname, op, stringop) \
    class classname: public expr_binary \
    { \
    public: \
        classname(expression *l, expression *r): expr_binary(l, r) { } \
        virtual long GetConst() const { return left->GetConst() op right->GetConst(); } \
    \
        virtual const std::string Dump() const \
        { return std::string("(") + left->Dump() + stringop + right->Dump() + ")"; } \
    };

unary_class(expr_bitnot, ~, "~")
unary_class(expr_negate, -, "-")

binary_class(expr_plus,   +, "+")
binary_class(expr_minus,  -, "-")
binary_class(expr_mul,    *, "*")
binary_class(expr_div,    /, "/")
binary_class(expr_shl,   <<, " shl ")
binary_class(expr_shr,   >>, " shr ")
binary_class(expr_bitand, &, " and")
binary_class(expr_bitor,  |, " or ")
binary_class(expr_bitxor, ^, " xor ")

void ReplaceLabelWithValue(const std::string&s, long value, expression*& e)
{
    if(expr_label *l = dynamic_cast<expr_label*> (e))
    {
        if(l->GetName() == s)
        {
            delete e;
            e = new expr_number(value);
            return;
        }
    }
    e->DefineLabel(s, value);
    if(e->IsConst())
    {
        value = e->GetConst();
        delete e;
        e = new expr_number(value);
    }
}

bool IsReservedWord(const std::string& s)
{
    const struct ins *insdata = std::lower_bound(ins, ins+InsCount, s);
    return (insdata < ins+InsCount && s == insdata->token)
         || s == ".byt"
         || s == ".word";
}

bool IsDelimiter(char c)
{
    return c == ':' || c == '\r' || c == '\n';
}

expression* RealParseExpression(ParseData& data, int prio=0)
{
    const int prio_addsub = 1; // + -
    const int prio_shifts = 2; // << >>
    const int prio_divmul = 3; // * /
    const int prio_bitand = 4; // &
    const int prio_bitor  = 5; // |
    const int prio_bitxor = 6; // ^
    const int prio_negate = 7; // -
    const int prio_bitnot = 8; // ~

    std::string s = ParseToken(data);
    
    expression* left = NULL;
    
    if(s.empty()) /* If no number or symbol */
    {
        char c = data.PeekC();
        if(c == '-') // negation
        {
            ParseData::StateType state = data.SaveState();
            left = RealParseExpression(data, prio_negate);
            data.GetC(); // eat
            if(!left) { data.LoadState(state); return left; }
            left = new expr_negate(left);
        }
        if(c == '~')
        {
            ParseData::StateType state = data.SaveState();
            data.GetC(); // eat
            left = RealParseExpression(data, prio_bitnot);
            if(!left) { data.LoadState(state); return left; }
            left = new expr_bitnot(left);
        }
        else if(c == '(')
        {
            ParseData::StateType state = data.SaveState();
            data.GetC(); // eat
            left = RealParseExpression(data, 0);
            data.SkipSpace();
            if(data.PeekC() == ')') data.GetC();
            else if(left) { delete left; left = NULL; }
            if(!left) { data.LoadState(state); return left; }
        }
        else
        {
            // invalid char
            return left;
        }
    }
    else if(s[0] == '-' || s[0] == '$' || (s[0] >= '0' && s[0] <= '9'))
    {
        long value = 0;
        bool negative = false;
        // Number.
        if(s[0] == '$')
        {
            unsigned pos = 1;
            if(s[1] == '-') { ++pos; negative = true; }
            
            for(; pos < s.size(); ++pos)
            {
                value = value*16;
                if(s[pos] >= 'A' && s[pos] <= 'F') value += 10+s[pos]-'A';
                if(s[pos] >= 'a' && s[pos] <= 'f') value += 10+s[pos]-'a';
                if(s[pos] >= '0' && s[pos] <= '9') value +=    s[pos]-'0';
            }
        }
        else
        {
            unsigned pos = 0;
            if(s[0] == '-') { negative = true; ++pos; }
            for(; pos < s.size(); ++pos)
                value = value*10 + (s[pos] - '0');
        }
        
        if(negative) value = -value;
        left = new expr_number(value);
    }
    else
    {
        if(IsReservedWord(s))
        {
            /* Attempt to use a reserved as variable name */
            return left;
        }
        
        left = new expr_label(s);
    }

    data.SkipSpace();
    if(!left) return left;
    
Reop:
    if(!data.EOF())
    {
        #define op2(reqprio, c1,c2, exprtype) \
            if(prio < reqprio && data.PeekC() == c1) \
            { \
                ParseData::StateType state = data.SaveState(); \
                bool ok = true; \
                if(c2 != 0) \
                { \
                    data.GetC(); char c = data.PeekC(); \
                    data.LoadState(state); \
                    ok = c == c2; \
                    if(ok) data.GetC(); \
                } \
                if(ok) \
                { \
                    data.GetC(); \
                    expression *right = RealParseExpression(data, reqprio); \
                    if(!right) \
                    { \
                        data.LoadState(state); \
                        return left; \
                    } \
                    left = new exprtype(left, right); \
                    if(left->IsConst()) \
                    { \
                        right = new expr_number(left->GetConst()); \
                        delete left; \
                        left = right; \
                    } \
                    goto Reop; \
            }   }
        
        op2(prio_addsub, '+',   0, expr_plus);
        op2(prio_addsub, '-',   0, expr_minus);
        op2(prio_divmul, '*',   0, expr_mul);
        op2(prio_divmul, '/',   0, expr_div);
        op2(prio_shifts, '<', '<', expr_shl);
        op2(prio_shifts, '>', '>', expr_shr);
        op2(prio_bitand, '&',   0, expr_bitand);
        op2(prio_bitor,  '|',   0, expr_bitor);
        op2(prio_bitxor, '^',   0, expr_bitxor);
    }
    return left;
}

struct ins_parameter
{
    char prefix;
    expression *exp;
    
    ins_parameter(): prefix(0), exp(NULL)
    {
    }
    
    ins_parameter(unsigned char num)
    : prefix(FORCE_LOBYTE), exp(new expr_number(num))
    {
    }
    
    // Note: No deletes here. Would cause problems when storing in vectors.
    
    tristate is_byte() const
    {
        if(prefix == FORCE_LOBYTE
        || prefix == FORCE_HIBYTE
        || prefix == FORCE_SEGBYTE)
        {
            return true;
        }
        if(prefix) return false;
        if(!exp->IsConst()) return maybe;
        long value = exp->GetConst();
        return value >= -0x80 && value < 0x100;
    }
    tristate is_word() const
    {
        if(prefix == FORCE_ABSWORD) return true;
        if(prefix) return false;
        if(!exp->IsConst()) return maybe;
        long value = exp->GetConst();
        return value >= -0x8000 && value < 0x10000;
    }
    tristate is_long() const
    {
        if(prefix == FORCE_LONG) return true;
        if(prefix) return false;
        if(!exp->IsConst()) return maybe;
        long value = exp->GetConst();
        return value >= -0x800000 && value < 0x1000000;
    }
    
    const std::string Dump() const
    {
        std::string result;
        if(prefix) result += prefix;
        if(exp) result += exp->Dump(); else result += "(nil)";
        return result;
    }
};

bool ParseExpression(ParseData& data, ins_parameter& result)
{
    data.SkipSpace();
    
    if(data.EOF()) return false;
    
    char prefix = data.PeekC();
    if(prefix == FORCE_LOBYTE
    || prefix == FORCE_HIBYTE
    || prefix == FORCE_ABSWORD
    || prefix == FORCE_LONG
    || prefix == FORCE_SEGBYTE
      )
    {
        // good prefix
        data.GetC();
    }
    else
    {
        // no prefix
        prefix = 0;
    }
    
    expression* e = RealParseExpression(data);
    
    if(e)
    {
        if(expr_minus *m = dynamic_cast<expr_minus *> (e))
        {
            // The rightmost element must be constant.
            if(m->right->IsConst())
            {
                // Convert into a sum.
                long value = m->right->GetConst();
                e = new expr_plus(m->left, new expr_number(-value));
                m->left = NULL; // so it won't be deleted
                delete m;
            }
        }
        
        if(!dynamic_cast<expr_number *> (e)
        && !dynamic_cast<expr_label *> (e))
        {
            if(expr_plus *p = dynamic_cast<expr_plus *> (e))
            {
                bool left_const = p->left->IsConst();
                bool right_const = p->right->IsConst();
                
                if(left_const && right_const)
                {
                    /* This should have been optimized */
                    std::fprintf(stderr, "Internal error\n");
                }
                else if((left_const && dynamic_cast<expr_label *> (p->right))
                    ||  (right_const && dynamic_cast<expr_label *> (p->left))
                       )
                {
                    if(left_const)
                    {
                        /* Reorder them so that the constant is always on right */
                        expression *tmp = p->left;
                        p->left = p->right;
                        p->right = tmp;
                    }
                    /* ok */
                }
                else
                    goto InvalidMath;
            }
            else
            {
    InvalidMath:
                /* Invalid pointer arithmetic */
                std::fprintf(stderr, "Invalid pointer arithmetic: '%s'\n",
                    e->Dump().c_str());
                delete e;
                e = NULL;
            }
        }
    }

    //std::fprintf(stderr, "ParseExpression returned: '%s'\n", result.Dump().c_str());
    
    result.prefix = prefix;
    result.exp    = e;
    
    return e != NULL;
}

class Object
{
    struct scope_t
    {
        std::map<std::string, unsigned> labels;
    };

    std::vector<struct scope_t> scope;
    std::vector<unsigned char> code;
    
    unsigned cur_scope;
    
    void CheckFixups()
    {
        // Resolve all fixups so far.
        // It's ok if not all are resolvable.
        
        // *TODO*
    }

public:
    Object(): cur_scope(0)
    {
    }
    
    void StartScope()
    {
        ++cur_scope;
        scope.resize(cur_scope);
    }

    void EndScope()
    {
        CheckFixups();
        scope.resize(--cur_scope);
    }
    
    unsigned GetScopeLevel() const { return cur_scope - 1; }

    void GenerateByte(unsigned char byte)
    {
        if(code.size() >= code.capacity())
        {
            code.reserve(code.size() + 256);
        }
        code.push_back(byte);
    }

    void AddFixup(char prefix, const std::string& ref, long value)
    {
        // *TODO*
    }
    
    void DefineLabel(const std::string& label)
    {
        std::string s = label;
        // Find out which scope to define it in
        unsigned scopenum = cur_scope-1;
        if(s[0] == '+')
        {
            // global label
            s = s.substr(1);
            scopenum = 0;
        }
        while(s[0] == '&' && cur_scope > 0)
        {
            s = s.substr(1);
            --scopenum;
        }
        scope[scopenum].labels[s] = code.size();
    }
};

tristate ParseAddrMode
   (ParseData& data,
    unsigned modenum,
    ins_parameter& p1,
    ins_parameter& p2)
{
    #define ParseReq(s) \
        for(const char *q = s; *q; ++q, data.GetC()) { \
            data.SkipSpace(); if(data.PeekC() != *q) return false; }
    #define ParseNotAllow(c) \
        data.SkipSpace(); if(data.PeekC() == c) return false
    #define ParseOptional(c) \
        
    #define ParseExpr(p) \
        if(!ParseExpression(data, p)) return false

    if(modenum >= AddrModeCount) return false;
    
    const AddrMode& modedata = AddrModes[modenum];
    
    if(modedata.forbid) { ParseNotAllow(modedata.forbid); }
    ParseReq(modedata.prereq);
    if(modedata.p1 != AddrMode::tNone) { ParseExpr(p1); }
    if(modedata.p2 != AddrMode::tNone) { ParseOptional(','); ParseExpr(p2); }
    ParseReq(modedata.postreq);
    
    data.SkipSpace();
    tristate result = data.EOF();
    switch(modedata.p1)
    {
        case AddrMode::tByte: result=result && p1.is_byte(); break;
        case AddrMode::tWord: result=result && p1.is_word(); break;
        case AddrMode::tLong: result=result && p1.is_long(); break;
        case AddrMode::tA: result=result && A_16bit ? p1.is_word() : p1.is_byte(); break;
        case AddrMode::tX: result=result && X_16bit ? p1.is_word() : p1.is_byte(); break;
        case AddrMode::tRel8: ;
        case AddrMode::tRel16: ;
        case AddrMode::tNone: ;
    }
    switch(modedata.p2)
    {
        case AddrMode::tByte: result=result && p2.is_byte(); break;
        case AddrMode::tWord: result=result && p2.is_word(); break;
        case AddrMode::tLong: result=result && p2.is_long(); break;
        case AddrMode::tA: result=result && A_16bit ? p2.is_word() : p2.is_byte(); break;
        case AddrMode::tX: result=result && X_16bit ? p2.is_word() : p2.is_byte(); break;
        case AddrMode::tRel8: ;
        case AddrMode::tRel16: ;
        case AddrMode::tNone: ;
    }
    return result;
}

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

struct OpcodeChoice
{
    typedef std::pair<unsigned, struct ins_parameter> paramtype;
    std::vector<paramtype> parameters;
    bool is_certain;
};

typedef std::vector<OpcodeChoice> ChoiceList;

void ParseIns(ParseData& data, Object& result)
{
MoreLabels:
    std::string tok;
    
    data.SkipSpace();
    
    if(IsDelimiter(data.PeekC()))
    {
        goto MoreLabels;
    }

    if(data.PeekC() == '+')
    {
        tok += data.GetC(); // defines global
    }
    else
    {
        while(data.PeekC() == '&') tok += data.GetC();
    }
    
    // Label may not begin with ., but instruction may
    if(tok.empty() && data.PeekC() == '.')
        tok += data.GetC();
    
    for(bool first=true;; first=false)
    {
        char c = data.PeekC();
        if(isalpha(c) || c == '_' || (!first && isdigit(c)))
            tok += data.GetC();
        else
            break;
    }
    
    std::fprintf(stderr, "Took token '%s'\n", tok.c_str());
    
    data.SkipSpace();
    
    std::vector<OpcodeChoice> choices;
    
    const struct ins *insdata = std::lower_bound(ins, ins+InsCount, tok);
    if(insdata == ins+InsCount || tok != insdata->token)
    {
        /* Other mnemonic */
        
        if(tok == ".byt")
        {
            OpcodeChoice choice;
            bool first=true, ok=true;
            for(;;)
            {
                data.SkipSpace();
                if(data.EOF()) break;
                
                if(first)
                    first=false;
                else
                {
                    if(data.PeekC() == ',') { data.GetC(); data.SkipSpace(); }
                }
                
                ins_parameter p;
                if(!ParseExpression(data, p) || !p.is_byte())
                {
                    /* FIXME: syntax error */
                    ok = false;
                    break;
                }
                choice.parameters.push_back(std::make_pair(1, p));
            }
            if(ok)
            {
                choice.is_certain = true;
                choices.push_back(choice);
            }
        }
        else if(tok == ".word")
        {
            OpcodeChoice choice;
            bool first=true, ok=true;
            for(;;)
            {
                data.SkipSpace();
                if(data.EOF()) break;
                
                if(first)
                    first=false;
                else
                {
                    if(data.PeekC() == ',') { data.GetC(); data.SkipSpace(); }
                }
                
                ins_parameter p;
                if(!ParseExpression(data, p)
                || !p.is_word())
                {
                    /* FIXME: syntax error */
                    ok = false;
                    break;
                }
                choice.parameters.push_back(std::make_pair(2, p));
            }
            if(ok)
            {
                choice.is_certain = true;
                choices.push_back(choice);
            }
        }
        else
        {
            std::fprintf(stderr, "Defined label '%s'\n", tok.c_str());
            result.DefineLabel(tok);
            goto MoreLabels;
        }
    }
    else
    {
        /* Found mnemonic */
        
        for(unsigned addrmode=0; insdata->opcodes[addrmode*3]; ++addrmode)
        {
            const std::string op(insdata->opcodes+addrmode*3, 2);
            if(op != "--")
            {
                ins_parameter p1, p2;
                
                const ParseData::StateType state = data.SaveState();
                
                tristate valid = ParseAddrMode(data, addrmode, p1, p2);
                if(!valid.is_false())
                {
                    if(op == "sb") result.StartScope();
                    else if(op == "eb") result.EndScope();
                    else if(op == "as") A_16bit = false;
                    else if(op == "al") A_16bit = true;
                    else if(op == "xs") X_16bit = false;
                    else if(op == "xl") X_16bit = true;
                    else
                    {
                        // Opcode in hex.
                        unsigned char opcode = std::strtol(op.c_str(), NULL, 16);
                        
                        OpcodeChoice choice;
                        unsigned op1size = GetOperand1Size(addrmode);
                        unsigned op2size = GetOperand2Size(addrmode);
                        
                        if(AddrModes[addrmode].p1 == AddrMode::tRel8)
                            p1.prefix = FORCE_REL8;
                        if(AddrModes[addrmode].p1 == AddrMode::tRel16)
                            p1.prefix = FORCE_REL16;
                        
                        choice.parameters.push_back(std::make_pair(1, opcode));
                        if(op1size)choice.parameters.push_back(std::make_pair(op1size, p1));
                        if(op2size)choice.parameters.push_back(std::make_pair(op2size, p2));

                        choice.is_certain = valid.is_true();
                        choices.push_back(choice);
                    }
                    
                /*
                    std::fprintf(stderr, "- %s mode %u (%s) (%u bytes)\n",
                        valid.is_true() ? "Is" : "Could be",
                        addrmode, op.c_str(),
                        GetOperandSize(addrmode)
                                );
                    std::fprintf(stderr, "  - p1=\"%s\"\n", p1.Dump().c_str());
                    std::fprintf(stderr, "  - p2=\"%s\"\n", p2.Dump().c_str());
                */
                }
                
                data.LoadState(state);
            }
            if(!insdata->opcodes[addrmode*3+2]) break;
        }
    }
    
    if(choices.empty())
    {
        std::fprintf(stderr, "Error in %s\n", data.GetRest().c_str());
        
        return;
    }

    /* Try to pick one the smallest of the certain choices */
    unsigned smallestsize = 0, smallestnum = 0;
    bool found=false;
    for(unsigned a=0; a<choices.size(); ++a)
    {
        const OpcodeChoice& c = choices[a];
        if(c.is_certain)
        {
            unsigned size = 0;
            for(unsigned b=0; b<c.parameters.size(); ++b)
                size += c.parameters[b].first;
            if(size < smallestsize || !found)
            {
                smallestsize = size;
                smallestnum = a;
                found = true;
            }
        }
    }
    if(!found)
    {
        /* If there were no certain choices, try to pick one of the uncertain ones. */
        for(unsigned a=0; a<choices.size(); ++a)
        {
            const OpcodeChoice& c = choices[a];
            
            unsigned sizediff = 0;
            for(unsigned b=0; b<c.parameters.size(); ++b)
            {
                int diff = 2 - (int)c.parameters[b].first;
                if(diff < 0)diff = -diff;
                sizediff += diff;
            }
            if(sizediff < smallestsize || !found)
            {
                smallestsize = sizediff;
                smallestnum = a;
                found = true;
            }
        }
    }
    if(!found)
    {
        std::fprintf(stderr, "Internal error\n");
    }
    
    const OpcodeChoice& c = choices[smallestnum];
    
    std::fprintf(stderr, "Choice %u:", smallestnum);
    for(unsigned b=0; b<c.parameters.size(); ++b)
    {
        long value = 0;
        std::string ref;
        
        unsigned size = c.parameters[b].first;
        const ins_parameter& param = c.parameters[b].second;
        
        const expression* e = param.exp;
        
        if(e->IsConst())
            value = e->GetConst();
        else if(const expr_label* l = dynamic_cast<const expr_label* > (e))
        {
            ref = l->GetName();
        }
        else if(const expr_plus* l = dynamic_cast<const expr_plus* > (e))
        {
            /* At this point, sum should have been organized
             * so that the const is on right hand and var on left.
             */
            ref   = (dynamic_cast<const expr_label *> (l->left)) ->GetName();
            value = l->right->GetConst();
        }
        else
        {
            fprintf(stderr, "Internal error - unknown node type.\n");
        }
        
        char prefix = param.prefix;
        if(!prefix)
            switch(size)
            {
                case 1: prefix = FORCE_LOBYTE; break;
                case 2: prefix = FORCE_ABSWORD; break;
                case 3: prefix = FORCE_LONG; break;
                default:
                       std::fprintf(stderr, "Internal error - unknown size: %u\n",
                        size);
            }
        
        
        if(!ref.empty()) result.AddFixup(param.prefix, ref, value);

        switch(param.prefix)
        {
            case FORCE_LONG:
                result.GenerateByte((value >> 16) & 0xFF);
                /* passthru */
            case FORCE_ABSWORD:
                result.GenerateByte((value >> 8) & 0xFF);
                /* passthru */
            case FORCE_LOBYTE:
                result.GenerateByte(value & 0xFF);
                break;
            case FORCE_REL8:
                result.GenerateByte(0x02);
                break;
            case FORCE_REL16:
                result.GenerateByte(0x03);
                break;
            case FORCE_HIBYTE:
                if(size != 1)
                {
                    fprintf(stderr, "Internal error - FORCE_HIBYTE and size %u\n", size);
                }
                result.GenerateByte((value >> 8) & 0xFF);
                break;
                
            case FORCE_SEGBYTE:
                if(size != 1)
                {
                    fprintf(stderr, "Internal error - FORCE_SEGBYTE and size %u\n", size);
                }
                result.GenerateByte((value >> 16) & 0xFF);
                break;
        }
        
        std::fprintf(stderr, " %s(%u)", param.Dump().c_str(), size);
    }
    if(c.is_certain)
        std::fprintf(stderr, " (certain)");
    std::fprintf(stderr, "\n");
    
    // *FIXME* choices not properly deallocated
}

static void ParserTest(Object& result, const std::string& s)
{
    ParseData data(s);
    ParseIns(data, result);
}

int main(void)
{
    Object obj;
    
    obj.StartScope();
    
    ParserTest(obj, "sta gruu");
    ParserTest(obj, "lda #gruu");
    ParserTest(obj, "ora $12");
    ParserTest(obj, "brl longkupo");
    ParserTest(obj, "bra shortkupo");
    
    obj.EndScope();
    
    return 0;
}

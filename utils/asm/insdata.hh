#include <string>

unsigned GetOperand1Size(unsigned modenum);
unsigned GetOperand2Size(unsigned modenum);
unsigned GetOperandSize(unsigned modenum);
bool IsReservedWord(const std::string& s);

struct AddrMode
{
    char forbid;
    char prereq[2];
    char postreq[6];
    enum { tNone, tByte, tWord, tLong, tA, tX, tRel8, tRel16 } p1, p2;
};
extern const struct AddrMode AddrModes[];
extern const unsigned AddrModeCount;

struct ins
{
    char token[6];
    char opcodes[25*3];

    bool operator< (const std::string &s) const { return s > token; }
};
extern const struct ins ins[];
extern const unsigned InsCount;

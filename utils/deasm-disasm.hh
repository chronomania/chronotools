#ifndef bqtDeasmDisasmHH
#define bqtDeasmDisasmHH

#include <map>

#include "tristate"

struct labeldata
{
    std::string name;
    tristate X_16bit, A_16bit;
    bool barrier;
    
    unsigned nextlabel;
    unsigned otherbranch;
    std::set<unsigned> referers;
    
    labeldata() : X_16bit(maybe), A_16bit(maybe), barrier(false),
                  nextlabel(0), otherbranch(0),
                  op1size(0), op2size(0)
    {
    }
    
    unsigned char opcode;
    unsigned addrmode;
    unsigned op1size, op1val;
    unsigned op2size, op2val;
    std::string op, param1, param2;
    
    bool IsImmed() const
    {
        switch(addrmode)
        {
            case 1: case 2: case 3: case 25: return true;
        }
        return false;
    }
    bool IsXaddr() const
    {
        switch(addrmode)
        {
            case 7: case 10: case 15: case 18: case 23: return true;
        }
        return false;
    }
    bool IsYaddr() const
    {
        switch(addrmode)
        {
            case 8: case 11: case 13: case 16: case 20: return true;
        }
        return false;
    }
    bool IsUnIndexedAddr() const
    {
        switch(addrmode)
        {
            case 6: case 14: case 17: return true;
        }
        return false;
    }
    bool IsIndirect() const
    {
        // Indirect is everything that has "(", "[" or "," in it
        switch(addrmode)
        {
            case 7: case 8:
            case 9: case 10: case 11: case 12: case 13:
            case 15: case 16:
            case 18: case 19: case 20:
            case 21: case 22: case 23: return true;
        }
        return false;
    }
};

extern std::map<unsigned, labeldata> labels;

extern void DefineLabel(unsigned address, const labeldata& label);
extern void DisAssemble();
extern void FixJumps();
extern void FixReps();

#endif

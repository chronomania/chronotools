#include <set>
#include <string>

#include "deasm-disasm.hh"
#include "rommap.hh"
#include "insdata.hh"
#include "assemble.hh"

static std::set<unsigned> labels_seen, labels_unseen;

class insdata
{
    std::string opcodes[256];
    unsigned addrmodes[256];
    
public:
    insdata()
    {
        for(unsigned a=0; a<InsCount; ++a)
        {
            std::string opcode = ins[a].token;
            if(opcode[0] == '.') continue;
            for(unsigned b=0; b<AddrModeCount; ++b)
            {
                unsigned opnum=256;
                const char *opptr = ins[a].opcodes + b*3;
                if(!*opptr) break;
                std::sscanf(opptr, "%X", &opnum);
                
                if(opnum < 256)
                {
                    opcodes[opnum]   = opcode;
                    addrmodes[opnum] = b;
                    
                    //printf("ins %02X opcode=%s addrmode=%u\n",
                    //    opnum, opcode.c_str(), b);
                }
            }
        }
    }
    const std::string& name(unsigned char x) const { return opcodes[x]; }
    const unsigned addrmode(unsigned char x) const { return addrmodes[x]; }
};

std::map<unsigned, labeldata> labels;

void DefineLabel(unsigned address, const labeldata& label)
{
    labels[address] = label;
    if(labels_seen.find(address) == labels_seen.end())
        labels_unseen.insert(address);
}

static void DisAssemble(unsigned address)
{
    static const insdata insdata;
    for(;;)
    {
        labels_unseen.erase(address);
        labels_seen.insert(address);
        
        const unsigned thisaddr = address;
        
        labeldata &label = labels[address];
        
        if(!label.X_16bit.is_maybe()) X_16bit = label.X_16bit; else label.X_16bit = X_16bit;
        if(!label.A_16bit.is_maybe()) A_16bit = label.A_16bit; else label.A_16bit = A_16bit;
        
        label.opcode   = ROM[address++ & 0x3FFFFF];
        label.op       = insdata.name(label.opcode);
        label.addrmode = insdata.addrmode(label.opcode);
        AddrMode mode = AddrModes[label.addrmode];
        
        label.param1 = mode.prereq;
        
        if(mode.p1 == AddrMode::tA) mode.p1 = A_16bit ? AddrMode::tWord : AddrMode::tByte;
        if(mode.p1 == AddrMode::tX) mode.p1 = X_16bit ? AddrMode::tWord : AddrMode::tByte;
        if(mode.p2 == AddrMode::tA) mode.p2 = A_16bit ? AddrMode::tWord : AddrMode::tByte;
        if(mode.p2 == AddrMode::tX) mode.p2 = X_16bit ? AddrMode::tWord : AddrMode::tByte;
        
        unsigned opsize = 0;
        unsigned opaddr = 0;
        
        switch(mode.p1)
        {
            case AddrMode::tByte:  opsize = 1; break;
            case AddrMode::tWord:  opsize = 2; break;
            case AddrMode::tLong:  opsize = 3; break;
            case AddrMode::tRel8:  opsize = 1; break;
            case AddrMode::tRel16: opsize = 2; break;
            default: opsize = 0;
        }
        for(unsigned p=0; p<opsize; ++p) { opaddr |= ROM[address++ & 0x3FFFFF] << (8*p); }
        
        if(mode.p1 == AddrMode::tRel8)
        {
            opaddr = address + (signed char) opaddr;
            opsize = 2;
        }
        else if(mode.p1 == AddrMode::tRel16)
        {
            opaddr = address + (signed short) opaddr;
            opsize = 2;
        }
        
        if(label.op == "jsr")
        {
            opaddr |= address & 0xFF0000;
        }
        
        if(opsize)
        {
            char Buf[64];
            std::sprintf(Buf, "$%0*X", opsize, opaddr);
            label.param1 += Buf;
            label.op1size = opsize,
            label.op1val  = opaddr;
        }
        
        if(label.op == "bcc" || label.op == "bcs"
        || label.op == "beq" || label.op == "bne"
        || label.op == "bmi" || label.op == "bpl"
        || label.op == "bra" || label.op == "brl"
        || label.op == "per" || label.op == "jmp"
        || label.op == "jsr" || label.op == "jsl")
        {
            // Indirect calls or jumps are not ok.
            if(!label.IsIndirect())
            {
                label.otherbranch = opaddr;
                
                if(label.op != "jsr"
                && label.op != "jsl"
                && label.op != "jmp")
                {
                    labels[opaddr].referers.insert(thisaddr);
                    labels[opaddr].X_16bit = X_16bit;
                    labels[opaddr].A_16bit = A_16bit;
                    
                    if(labels_seen.find(opaddr) == labels_seen.end())
                    {
                        labels_unseen.insert(opaddr);
                    }
                }
            }
        }
        if(label.op == "rep")
        {
            if(opaddr & 0x10) X_16bit = true;
            if(opaddr & 0x20) A_16bit = true;
        }
        if(label.op == "sep")
        {
            if(opaddr & 0x10) X_16bit = false;
            if(opaddr & 0x20) A_16bit = false;
        }

        opaddr = 0;
        switch(mode.p2)
        {
            case AddrMode::tByte: opsize = 1; break;
            default: opsize = 0;
        }
        for(unsigned p=0; p<opsize; ++p) { opaddr |= ROM[address++ & 0x3FFFFF] << (8*p); }
        if(opsize)
        {
            char Buf[64];
            std::sprintf(Buf, "$%0*X", opsize, opaddr);
            label.param2 += Buf;
            label.op2size = opsize,
            label.op2val  = opaddr;
        }
        
        label.param1 += mode.postreq;
        
        if(label.op == "rts"
        || label.op == "bra"
        || label.op == "brl"
        || label.op == "jmp")
        {
            label.barrier = true;
            break;
        }
        
        label.nextlabel = address;
        labels[address].referers.insert(thisaddr);
    }
}

void DisAssemble()
{
    while(!labels_unseen.empty())
    {
        DisAssemble(*labels_unseen.begin());
    }
}

void FixJumps()
{
    // Find a label.
    for(std::map<unsigned, labeldata>::iterator
        i = labels.begin();
        i != labels.end();
        ++i)
    {
        unsigned address = i->first;
        labeldata& label = i->second;
        
        if((label.op == "bra" || label.op == "brl" || label.op == "jmp")
         && !label.IsIndirect()
         //&& label.otherbranch == label.op1val
           )
        {
            /*
            printf("rerouting: %s %s %s (next=$%X, other=$%X, op1=$%X) at $%X\n",
                label.op.c_str(),
                label.param1.c_str(),
                label.param2.c_str(),
                label.nextlabel,
                label.otherbranch,
                label.op1val,
                address);
            */
            unsigned target = label.otherbranch;
            label.nextlabel   = target;
            label.otherbranch = 0;
            
            for(std::set<unsigned>::const_iterator
                j = label.referers.begin();
                j != label.referers.end();
                ++j)
            {
                unsigned sourceaddr = *j;
                
                labeldata& referer = labels[sourceaddr];
                
                if(referer.nextlabel == address) referer.nextlabel = target;
                if(referer.otherbranch == address) referer.otherbranch = target;
           }
        }
    }
}

void FixReps()
{
    // Find reps and convert/delete them.
    for(std::map<unsigned, labeldata>::iterator
        i = labels.begin();
        i != labels.end();
        ++i)
    {
        unsigned address = i->first;
        labeldata& label = i->second;
        
        if(label.op == "rep" || label.op == "sep")
        {
            unsigned p = label.op1val;
            
            p &= ~0x30;
            
            if(p == 1)
            {
                label.op       = label.op == "sep" ? "sec" : "clc";
                label.param1   = "";
                label.op1size = label.op1val = label.addrmode = 0;
                continue;
            }
            if(!p)
            {
                // Defining to "nop" is easier than deleting it.
                label.op       = "nop (was: "+label.op+")";
                label.param1   = "";
                label.op1size = label.op1val = label.addrmode = 0;
            }
        }
    }
}

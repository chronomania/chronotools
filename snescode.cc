#include <list>
#include <string>
#include <cstdarg>

#include "snescode.hh"

using std::list;
using std::string;

typedef unsigned char byte;
SNEScode::SNEScode(const vector<unsigned char>& data)
  : vector<unsigned char> (data),
    located(false)
{
}

SNEScode::SNEScode(): located(false)
{
}
SNEScode::~SNEScode()
{
}

void SNEScode::YourAddressIs(unsigned addr)
{
    address = addr;
    located = true;
    
    //fprintf(stderr, "%u-bytes long subroutine... placing\n", size());
    
#if 0
    list<FarToNearCallCode *>::iterator i;
    for(i=farcalls.begin(); i!=farcalls.end(); ++i)
    {
        FarToNearCallCode *ff = *i;
        ff->Finish(*this, addr);
        delete ff;
    }
#endif

#if 0
    fprintf(stderr, "%u-bytes long subroutine placed at $%06X\n", size(), addr);
#endif
    
#if 0
    farcalls.clear();
#endif
}

void SNEScode::AddCallFrom(unsigned addr) { callfrom.insert(addr); }
void SNEScode::AddLongPtrFrom(unsigned addr) { longptrfrom.insert(addr); }
void SNEScode::AddOffsPtrFrom(unsigned addr) { offsptrfrom.insert(addr); }
void SNEScode::AddPagePtrFrom(unsigned addr) { pageptrfrom.insert(addr); }

#include "ctinsert.hh"
#include "logfiles.hh"

void insertor::PlaceData(const vector<unsigned char>& data, unsigned address)
{
    SNEScode imgdata(data);
    imgdata.YourAddressIs(address);
    codes.push_back(imgdata);
}

void insertor::PlaceByte(unsigned char byte, unsigned address)
{
    SNEScode imgdata;
    //imgdata.EmitCode(byte);
    imgdata.push_back(byte);
    imgdata.YourAddressIs(address & 0x3FFFFF);
    codes.push_back(imgdata);
}

//////////// Obsolete code
#if 0
class FarToNearCallCode
{
    unsigned ret_addr, rtl_addr, targetpos, segment;
    
    static unsigned GetRTLaddr(unsigned segment)
    {
        switch(segment)
        {
            // Add list of RTL offsets in different segmets here.
            case 0xC2: return 0x5841;
            
            default:
                fprintf(stderr, "Error: RTL location in segment 0x%02X unknown\n",
                    segment);
                return 0;
        }
    }
    void Patch(SNEScode &code, unsigned offs, unsigned value)
    {
        //fprintf(stderr, "Patching %u with %04X\n", offs, value);
        code[offs + 0] = (value     ) & 255;
        code[offs + 1] = (value >> 8) & 255;
    }
public:
    FarToNearCallCode(SNEScode &code) : ret_addr(0),rtl_addr(0),targetpos(0),segment(0)
    {
        //fprintf(stderr, "fnc init\n");
        
        code.EmitCode(0x4B);       //phk (push pb)
        code.Set16bit_X();         //we need 16-bit immeds here
        code.EmitCode(0xA0, 0,0);  //ldy immed16
        ret_addr = code.size() - 2;
        code.EmitCode(0x5A);       //phy

        code.EmitCode(0xA0, 0,0);  //ldy immed16
        rtl_addr = code.size() - 2;
        code.EmitCode(0x5A);       //phy
    }
    void Proceed(SNEScode &code, unsigned target)
    {
        //fprintf(stderr, "fnc proc(0x%06X)\n", target);
        segment = (target>>16) & 255;
        code.EmitCode(0x5C, target&255, (target>>8)&255, segment);
        targetpos = code.size();
    }
    void Finish(SNEScode &code, unsigned address)
    {
        //fprintf(stderr, "fnc fini(0x%06X)\n", address);
        Patch(code, rtl_addr, GetRTLaddr(segment) - 1);
        Patch(code, ret_addr, targetpos + address - 1);
    }
};

void SNEScode::Add(unsigned n...)
{
    if(n > 1) reserve(size() + n);
    va_list ap;
    va_start(ap, n);
    while(n > 0)
    {
        byte a = va_arg(ap, unsigned);
        push_back(a);
        --n;
    }
    va_end(ap);
}

void SNEScode::EmitCode(byte a)
{ CheckStates(); push_back(a); }
void SNEScode::EmitCode(byte a,byte b)
{ CheckStates(); Add(2, a,b); }
void SNEScode::EmitCode(byte a,byte b,byte c)
{ CheckStates(); Add(3, a,b,c); }
void SNEScode::EmitCode(byte a,byte b,byte c,byte d)
{ CheckStates(); Add(4, a,b,c,d); }
void SNEScode::EmitCode(byte a,byte b,byte c,byte d,byte e)
{ CheckStates(); Add(5, a,b,c,d,e); }
void SNEScode::EmitCode(byte a,byte b,byte c,byte d,byte e,byte f)
{ CheckStates(); Add(6, a,b,c,d,e,f); }

void SNEScode::BitnessUnknown()
{
    bits.StateX = bits.StateM = BitnessData::StateUnknown;
}

void SNEScode::BitnessAnything()
{
    bits.StateX = bits.StateM = BitnessData::StateAnything;
}

void SNEScode::CheckStates()
{
    unsigned rep = 0;
    unsigned sep = 0;
    
    if(bits.WantedX != BitnessData::StateUnknown
    && bits.WantedX != bits.StateX)
    {
        if(bits.WantedX == BitnessData::StateSet) sep |= 0x10; else rep |= 0x10;
        bits.StateX = bits.WantedX;
    }

    if(bits.WantedM != BitnessData::StateUnknown
    && bits.WantedM != bits.StateM)
    {
        if(bits.WantedM == BitnessData::StateSet) sep |= 0x20; else rep |= 0x20;
        bits.StateM = bits.WantedM;
    }

    if(rep) EmitCode(0xC2, rep); // rep n
    if(sep) EmitCode(0xE2, sep); // sep n
}

void SNEScode::Set16bit_M()
{
    bits.WantedM = BitnessData::StateUnset;
}

void SNEScode::Set8bit_M()
{
    bits.WantedM = BitnessData::StateSet;
}

void SNEScode::Set16bit_X()
{
    bits.WantedX = BitnessData::StateUnset;
}

void SNEScode::Set8bit_X()
{
    bits.WantedX = BitnessData::StateSet;
}

SNEScode::FarToNearCall::FarToNearCall(SNEScode &c, FarToNearCallCode *p): code(c), ptr(p)
{
}

void SNEScode::FarToNearCall::Proceed(unsigned addr)
{
    ptr->Proceed(code, addr);
}

const SNEScode::FarToNearCall SNEScode::PrepareFarToNearCall()
{
    FarToNearCallCode *f = new FarToNearCallCode(*this);
    farcalls.push_back(f);
    return FarToNearCall(*this, f);
}

SNEScode::RelativeBranch::RelativeBranch(SNEScode &c) : code(c),target(0)
{
}
void SNEScode::RelativeBranch::FromHere()
{
    if(from.empty()) bits = code.bits; else bits.Update(code.bits);
    from.insert(code.size()-1);
}

void SNEScode::RelativeBranch::ToHere()
{
    bits.Update(code.bits);
    code.bits.StateX = bits.StateX;
    code.bits.StateM = bits.StateM;
    target = code.size();
}

void SNEScode::RelativeBranch::Proceed()
{
    set<unsigned>::const_iterator i;
    for(i=from.begin(); i!=from.end(); ++i)
    {
        unsigned pos = *i;
        int value = target - pos - 1; // negative also ok
        
        // target = (pos-1) + value + 2;
        //   - therefore, target-2-pos+1 = value = target-pos-1
        
        if(value > 127 || value < -128)
            fprintf(stderr, "ERROR: Relative branch from %X to %X (value %d) out of range\n",
                pos, target, value);
        
        code[pos] = value;
    }
    from.clear();
}

const SNEScode::RelativeBranch SNEScode::PrepareRelativeBranch()
{
    return RelativeBranch(*this);
}

SNEScode::RelativeLongBranch::RelativeLongBranch(SNEScode &c) : code(c),target(0)
{
}

void SNEScode::RelativeLongBranch::FromHere()
{
    if(from.empty()) bits = code.bits; else bits.Update(code.bits);
    from.insert(code.size()-2);
}

void SNEScode::RelativeLongBranch::ToHere()
{
    bits.Update(code.bits);
    code.bits.StateX = bits.StateX;
    code.bits.StateM = bits.StateM;
    target = code.size();
}

void SNEScode::RelativeLongBranch::Proceed()
{
    set<unsigned>::const_iterator i;
    for(i=from.begin(); i!=from.end(); ++i)
    {
        unsigned pos = *i;
        int value = target - pos - 2; // negative also ok
        
        // target = (pos-1) + value + 3;
        //   - therefore, target-3-pos+1 = value = target-pos-2

        if(value > 32767 || value < -32768)
            fprintf(stderr, "ERROR: Relative branch from %X to %X (value %d) out of range\n",
                pos, target, value);
        
        code[pos] = (unsigned char) (value & 255);
        code[pos+1]=(unsigned char) (value >> 8);
    }
    from.clear();
}

const SNEScode::RelativeLongBranch SNEScode::PrepareRelativeLongBranch()
{
    return RelativeLongBranch(*this);
}

void SubRoutine::CallFar(unsigned address)
{
    code.EmitCode(0x22,
        address&255,
        (address>>8) & 255,
        (address>>16) & 255);
}
void SubRoutine::JmpFar(unsigned address)
{
    code.EmitCode(0x5C,
        address&255,
        (address>>8) & 255,
        (address>>16) & 255);
}

void SubRoutine::CallSub(const ucs4string &name)
{
    CallFar(0);
    requires[name].insert(code.size()-3);
    code.BitnessUnknown();
}

void FunctionList::Define(const ucs4string &name, const SubRoutine &sub)
{
    functions[name] = make_pair(sub, false);
}

void FunctionList::RequireFunction(const ucs4string &name)
{
    functions_t::iterator i = functions.find(name);
    if(i == functions.end())
    {
        fprintf(stderr, "Error: Function '%s' not defined!\n",
            WstrToAsc(name).c_str());
        return;
    }
    bool &required         = i->second.second;
    const SubRoutine &func = i->second.first;
    
    if(required) return;
#if 0
    fprintf(stderr, "Requiring function '%s' (%u bytes)...\n",
        WstrToAsc(name).c_str(),
        func.code.size());
#endif
    required = true;
    
    SubRoutine::requires_t::const_iterator j;
    for(j = func.requires.begin();
        j != func.requires.end();
        ++j)
    {
        RequireFunction(j->first);
    }
}

void insertor::LinkAndLocate(FunctionList &Functions)
{
    FILE *log = GetLogFile("mem", "log_addrs");
    
    vector<SNEScode>   codeblobs;
    vector<ucs4string> funcnames;
    
    for(FunctionList::functions_t::const_iterator
        i = Functions.functions.begin();
        i != Functions.functions.end();
        ++i)
    {
        // Omit nonrequired functions.
        if(!i->second.second) continue;
        
        codeblobs.push_back(i->second.first.code);
        funcnames.push_back(i->first);
    }
        
    vector<freespacerec> blocks(codeblobs.size());
    for(unsigned a=0; a<codeblobs.size(); ++a)
        blocks[a].len = codeblobs[a].size();
    
    freespace.OrganizeToAnyPage(blocks);
    
    if(log)
        fprintf(log, "Placing %u functions:\n", codeblobs.size());
    
    for(unsigned a=0; a<codeblobs.size(); ++a)
    {
        unsigned addr = blocks[a].pos;
        codeblobs[a].YourAddressIs(addr);
        
        if(log)
            fprintf(log, "  Function %s (%u bytes) will be placed at $%06X\n",
                WstrToAsc(funcnames[a]).c_str(),
                codeblobs[a].size(),
                addr | 0xC00000);
    }
    
    // All of them are now placed somewhere.
    // Link them!
    
    for(unsigned a=0; a<codeblobs.size(); ++a)
    {
        FunctionList::functions_t::const_iterator i = Functions.functions.find(funcnames[a]);
        
        const SubRoutine::requires_t &req = i->second.first.requires;
        for(SubRoutine::requires_t::const_iterator
            j = req.begin();
            j != req.end();
            ++j)
        {
            // Find the address of the function we're requiring
            unsigned req_addr = NOWHERE;
            for(unsigned b=0; b<funcnames.size(); ++b)
                if(funcnames[b] == j->first)
                {
                    req_addr = codeblobs[b].GetAddress() | 0xC00000;
                    break;
                }
            
            for(set<unsigned>::const_iterator
                k = j->second.begin();
                k != j->second.end();
                ++k)
            {
                codeblobs[a][*k + 0] = req_addr & 255;
                codeblobs[a][*k + 1] = (req_addr >> 8) & 255;
                codeblobs[a][*k + 2] = req_addr >> 16;
            }
        }
    }
    
    // They have now been linked.
    
    codes.insert(codes.begin(), codeblobs.begin(), codeblobs.end());
}
#endif
/// End obsolete code

#include "o65.hh"
#include "config.hh"
void insertor::LinkCalls(const string& section, const O65& code)
{
    const ConfParser::ElemVec& elems = GetConf(section.c_str(), "add_call_of").Fields();
    for(unsigned a=0; a<elems.size(); a += 4)
    {
        const ucs4string& funcname = elems[a];
        unsigned address           = elems[a+1];
        unsigned nopcount          = elems[a+2];
        bool add_rts               = elems[a+3];
        
        SNEScode tmpcode;
        tmpcode.AddCallFrom(address);
        tmpcode.YourAddressIs(code.GetSymAddress(WstrToAsc(funcname)));
        codes.push_back(tmpcode);
        
        address += 4;
        if(add_rts)
        {
            PlaceByte(0x60, address++);
            if(nopcount > 0)
            {
                freespace.Add((address >> 16) & 0x3F,
                              address & 0xFFFF,
                              nopcount);
                
                // Don't initialize, or it will overwrite whatever
                // uses that space!
                //while(skipbytes-- > 0) { PlaceByte(0, address++); --nopcount; }
                nopcount = 0;
            }
        }
        else
        {
            while(nopcount > 2)
            {
                nopcount -= 2;
                unsigned skipbytes = nopcount;
                if(skipbytes > 127) skipbytes = 127;
                PlaceByte(0x80,      address++); // BRA over the space.
                PlaceByte(skipbytes, address++);
                
                freespace.Add((address >> 16) & 0x3F,
                              address & 0xFFFF,
                              skipbytes);
                
                // Don't initialize, or it will overwrite whatever
                // uses that space!
                //while(skipbytes-- > 0) { PlaceByte(0, address++); --nopcount; }
                nopcount -= skipbytes;
            }
            while(nopcount > 0) { PlaceByte(0xEA, address++); --nopcount; }
        }
    }
}

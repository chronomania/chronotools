#include <cstdio>
#include <string>
#include <stack>
#include <cctype>
#include <cstdarg>
#include <algorithm>
#include <set>
#include <map>

#include "insdata.hh"
#include "rommap.hh"
#include "tristate"
#include "autoptr"

//dummy, used by rommap.o
void PutAscii(const string& ) {}
void BlockComment(const string& ) {}
void StartBlock(const char*, const string&, unsigned=0) {}
void EndBlock() {}
std::FILE *GetLogFile(const char*, const char*){return NULL;}

bool A_16bit;
bool X_16bit;

#undef BIT

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
                std::sscanf(ins[a].opcodes + b*3, "%X", &opnum);
                
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

namespace
{
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

    std::map<unsigned, labeldata> labels;
    std::set<unsigned> labels_seen, labels_unseen;
    
    void DefineLabel(unsigned address, const labeldata& label)
    {
        labels[address] = label;
        if(labels_seen.find(address) == labels_seen.end())
            labels_unseen.insert(address);
    }
    
    void DisAssemble(unsigned address)
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

    void DefinePointers()
    {
        for(unsigned x=0; x<256; ++x)
        {
            unsigned ptraddr = 0xC05D6E + x*2;
            
            unsigned ptr = ROM[(ptraddr+0) & 0x3FFFFF]
                        | (ROM[(ptraddr+1) & 0x3FFFFF] << 8);
            
            unsigned codeaddr = ptr + (ptraddr & 0xFF0000);
            
            //printf("ptr %02X at %X -> %X\n", x, ptraddr, codeaddr);
            
            char Buf[64];
            sprintf(Buf, "Op_%02X", x);
            
            labeldata label;
            label.name = Buf;
            label.X_16bit    = true;
            label.A_16bit    = false;
            DefineLabel(codeaddr, label);
        }
    }

    struct Output
    {
        std::string code;
        bool A_modified;
        bool X_modified;
        bool Y_modified;
        bool D_modified;
        bool B_modified;
        bool terminated;
        std::set<unsigned> modified_mem;
        std::set<unsigned> labels_seen;
        bool dummy;
        
        Output(): A_modified(false),
                  X_modified(false),
                  Y_modified(false),
                  D_modified(false),
                  B_modified(false),
                  terminated(false),
                  dummy(false)
        {
        }
        
        void Clear(bool DummyRun)
        {
            A_modified = X_modified = Y_modified = D_modified = B_modified = false;
            terminated = false;
            dummy = DummyRun;
            modified_mem.clear();
            code = "";
        }
    };
    
/*
    struct Expr: public ptrable
    {
        bool is_16bit;
    };
    struct Expr: public 
*/
    struct Machine
    {
        class Value;
        struct Register;
        struct Compare;
        
        class Value
        {
        public:
            enum addrmode { Zero, StackZero, Data, Abs };
            
            Value(): is_16bit(maybe)
            {
                Undefine();
            }
            void Set(int num)
            {
                Undefine();
                char Buf[64];
                if(num < 15)
                    std::sprintf(Buf, "%d", num);
                else
                    std::sprintf(Buf, "$%X", num);
                value    = Buf;
                is_const = true;
            }
            
            void Add(int num)
            {
                int OldAdd = 0;
                
                unsigned pos = value.size();
                while(pos > 0 && std::isdigit(value[--pos]))
                    OldAdd = OldAdd*10 + (value[pos]-'0');
                if(value[pos] == '+'
                || value[pos] == '-')
                {
                    if(value[pos] == '-') OldAdd = -OldAdd;
                    value.erase(value.begin()+pos, value.end());
                }
                
                OldAdd += num;
                if(!OldAdd) return;
                
                char Buf[64];
                std::sprintf(Buf, "%+d", OldAdd);
                value += Buf;
            }

            void And(const Value& v)
            {
                value = Display() + " & " + v.Display();
                is_const = is_const && v.is_const;
            }
            void Sub(const Value& v)
            {
                value = Display() + " - " + v.Display();
                is_const = is_const && v.is_const;
            }
            void Add(const Value& v)
            {
                value = Display() + " + " + v.Display();
                is_const = is_const && v.is_const;
            }
            void Or(const Value& v)
            {
                value = Display() + " | " + v.Display();
                is_const = is_const && v.is_const;
            }
            void Mul(const Value& v)
            {
                value = Display() + " * " + v.Display();
                is_const = is_const && v.is_const;
            }
            void Div(const Value& v)
            {
                value = Display() + " / " + v.Display();
                is_const = is_const && v.is_const;
            }
            void Mod(const Value& v)
            {
                value = Display() + " mod " + v.Display();
                is_const = is_const && v.is_const;
            }
            
            void SetL(unsigned addr, addrmode where) // [addr]
            {
                Undefine();
                value = "[";
                value = value + BuildAddr(addr, where) + "]";
                is_const = false;
            }
            void SetLL(unsigned addr, addrmode where) // [word[addr]]
            {
                Undefine();
                value = "[word[";
                value = value + BuildAddr(addr, where) + "]]";
                is_const = false;
            }
            void SetRL(unsigned addr,  addrmode where, Register& reg) // [addr+reg]
            {
                Undefine();
                value = "[";
                value = value + BuildAddr(addr, where) + "+" + reg.UseValue() + "]";
                Depend(reg);
                is_const = false;
            }
            void SetRLL(unsigned addr, addrmode where, Register& reg) // [word[addr+reg]]
            {
                Undefine();
                value = "[word[";
                value = value + BuildAddr(addr, where) + "+" + reg.UseValue() + "]]";
                Depend(reg);
                is_const = false;
            }
            void SetLRL(unsigned addr, addrmode where, Register& reg) // [word[addr]+reg]
            {
                Undefine();
                value = "[word[";
                value = value + BuildAddr(addr, where) + "]+" + reg.UseValue() + "]";
                Depend(reg);
                is_const = false;
            }
            void LSetLL(unsigned addr, addrmode where) // [long[addr]]
            {
                Undefine();
                value = "[long[";
                value = value + BuildAddr(addr, where) + "]]";
                is_const = false;
            }
            void LSetLRL(unsigned addr, addrmode where, Register& reg) // [long[addr]+reg]
            {
                Undefine();
                value = "[long[";
                value = value + BuildAddr(addr, where) + "]+" + reg.UseValue() + "]";
                Depend(reg);
                is_const = false;
            }
            
            const std::string Display() const
            {
                const std::string& v = value;
                
                /*
                for(std::set<const Register*>::const_iterator
                    i = deps.begin(); i != deps.end(); ++i)
                {
                    v += "{dep:" + (*i)->GetName() + "}";
                }
                */
                
                if(!IsConst())
                {
                    if(is_16bit.is_true()) return "word(" + v + ")";
                    if(is_16bit.is_false()) return "byte(" + v + ")";
                }
                return v;
            }
            
            void SetRegister(const Register& reg)
            {
                Undefine();
                value    = reg.GetName();
                is_const = false;
                Depend(reg);
            }
            
            void Undefine()
            {
                is_16bit = maybe;
                is_const = false;
                value    = "?";
                deps.clear();
            }

            void Undepend(const Register& reg)
            {
                deps.erase(&reg);
            }
            
            bool Depends(const Register& reg) const
            {
                return deps.find(&reg) != deps.end();
            }
            
            void SetBitness(tristate v)
            {
                is_16bit = v;
            }
            
            void SetString(const std::string& n)
            {
                value = n;
                is_const = true;
            }
            
            bool operator!= (const Value& b) const
            {
                return !operator== (b);
            }
            bool operator== (const Value& b) const
            {
                return value == b.value
                    && is_16bit == b.is_16bit;
            }
            
            bool IsConst() const { return is_const; }
            
            tristate is_16bit;
            
        private:
            std::string value;
            bool is_const;
            
            const std::string BuildAddr(unsigned addr, addrmode where) const
            {
                char Buf[64];
                switch(where)
                {
                    case Zero: std::sprintf(Buf, "$%02X", addr); break;
                    case StackZero: std::sprintf(Buf, "$%02X+S", addr); break;
                    case Data: std::sprintf(Buf, "$%04X", addr); break;
                    case Abs: std::sprintf(Buf, "$%06X", addr); break;
                }
                return Buf;
            }
            
            void Depend(const Register& reg)
            {
                if(!reg.defined) deps.insert(&reg);
            }

            std::set<const Register*> deps;
        };
        
        struct Register
        {
            Value value;
            mutable bool displayed;
            bool defined;
            Machine& machine;
            
            // defined-flag is set to "true" here so that
            // the NewValue() call in Undefine() won't do anything.
            Register(Machine& m): defined(true), machine(m)
            {
                Undefine();
            }
            Register(Machine& m, const Register& b)
             : value(b.value), displayed(b.displayed),
               defined(b.displayed), machine(m)
            {
            }
            void operator=(const Register& b)
            {
                //machine.UndependRegs(*this);

                value     = b.value;
                displayed = b.displayed;
                defined   = b.defined;
            }
            
            void Undefine()
            {
                NewValue();
                
                //machine.printf("<Undefine %s>\n", GetName().c_str());
                displayed = false;
                defined   = false;
                
                value.SetRegister(*this);
            }
            
            const std::string GetName() const
            {
                return machine.GetRegisterName(*this);
            }
            
            void Add(int num)
            {
                NewValue();

                value.Add(num);
                Loaded();
                machine.carry = maybe;
            }
            void Sub(int num)
            {
                Add(-num);
            }
            void Add(const labeldata& label, tristate bitness)
            {
                NewValue();

                value.Add(BuildValue(label));
                value.SetBitness(bitness);
                Loaded();
                machine.carry = maybe;
            }
            void Sub(const labeldata& label, tristate bitness)
            {
                NewValue();

                value.Sub(BuildValue(label));
                value.SetBitness(bitness);
                Loaded();
                machine.carry = maybe;
            }
            void And(const labeldata& label, tristate bitness)
            {
                NewValue();

                value.And(BuildValue(label));
                value.SetBitness(bitness);
                Loaded();
            }
            void Or(const labeldata& label, tristate bitness)
            {
                NewValue();

                value.Or(BuildValue(label));
                value.SetBitness(bitness);
                Loaded();
            }
            
            void Load(const labeldata& label, tristate bitness)
            {
                if(!label.IsImmed())
                {
                    // immeds are usually stored somewhere,
                    // so don't flush it right now...
                    NewValue();
                }

                value = BuildValue(label);
                value.SetBitness(bitness);
                Loaded();
            }
            void Load(const Value& v, tristate bitness)
            {
                NewValue();

                value = v;
                value.SetBitness(bitness);
                Loaded();
            }
            void Load(const Register& reg, tristate bitness)
            {
                NewValue();

                Load(reg.value, bitness);
                Loaded();
            }
            void Compare(const labeldata& label, tristate bitness)
            {
                Loaded();
                machine.compare.Op2 = BuildValue(label);
                machine.compare.Op2.SetBitness(bitness);
                machine.compare.type = Compare::CMP;
                machine.carry = maybe;
            }
            void Bit(const labeldata& label, tristate bitness)
            {
                Loaded();
                machine.compare.Op2 = BuildValue(label);
                machine.compare.Op2.SetBitness(bitness);
                machine.compare.type = Compare::BIT;
            }
            const std::string Display() const
            {
                // Vasta silloin kun rekisterin uutta arvoa tarvitaan,
                // viittaukset vanhaan arvoon puretaan.
                //machine.UndependRegs(*this);
                
                displayed = true;
                return value.Display();
            }
            const std::string UseValue()
            {
                if(false && defined && !value.IsConst())
                {
                    // If this register contains a value of memory address
                    // and is going to be used in referring to another memory
                    // address, flush the assignment and refer to the register
                    // name only.
                    
                    machine.printf(
                        ";%s deflashing for memory reference\n",
                        GetName().c_str());
                    
                    machine.DisplayRegister(*this);
                    Undefine();
                }
                return value.Display();
            }
            void DontDepend(const Register& reg)
            {
                if(this != &reg && value.Depends(reg))
                {
                    /**/
                    machine.printf(
                        ";%s undepending because of %s\n",
                        GetName().c_str(),
                        reg.GetName().c_str());
                    /**/
                    value.Undepend(reg); // prevent a loop in DisplayRegister()
                    
                    machine.DisplayRegister(*this);
                    Undefine();
                }
            }
        private:
            const Value BuildValue(const labeldata& label) const
            {
                return machine.ParseAddress(label);
            }
            void Loaded()
            {
                displayed = false;
                defined   = true;
                
                machine.compare.Op1 = this;
                machine.compare.type = Compare::AUTO;
            }
            void NewValue()
            {
                if(!defined) machine.UndependRegs(*this);
            }
            
            //void operator= (const Register&);
            //Register(const Register&);
        };
        struct Compare
        {
            const Register* Op1;
            Value Op2;
            
            enum { CMP, AUTO, BIT } type;
            
            Compare(): Op1(NULL), type(AUTO) { }
        };
        
        Compare compare;
        Register A, X, Y, D, B;
        std::stack<Value> stack;
        Output output;
        tristate carry;
        
        typedef std::map<unsigned, Value> memorymap;
        memorymap memory;
        enum { multiply, divide } calcmode;
    private:
        mutable int indent;
    public:
        Machine(): A(*this), X(*this), Y(*this),
                   D(*this), B(*this),
                   carry(maybe),
                   indent(0)
        {
        }
        Machine(const Machine& b)
        : compare(b.compare),
          A(*this, b.A),
          X(*this, b.X),
          Y(*this, b.Y),
          D(*this, b.D),
          B(*this, b.B),
          stack(b.stack),
          carry(b.carry),
          memory(b.memory),
          indent(b.indent)
        {
            if(b.compare.Op1 == &b.A) compare.Op1 = &A;
            if(b.compare.Op1 == &b.X) compare.Op1 = &X;
            if(b.compare.Op1 == &b.Y) compare.Op1 = &Y;
            if(b.compare.Op1 == &b.D) compare.Op1 = &D;
            if(b.compare.Op1 == &b.B) compare.Op1 = &B;
        }
        void Store(Register& reg, const labeldata& label, tristate bitness)
        {
            /**/
            if(label.IsUnIndexedAddr())
            {
                output.modified_mem.insert(label.op1val);
                //reg.UseValue();
                
                memory[label.op1val] = reg.value;
                memory[label.op1val].SetBitness(bitness);

                if(label.op1val == 0x4202 || label.op1val == 0x4203)
                {
                    calcmode = multiply;
                }
                if(label.op1val == 0x4204 || label.op1val == 0x4205)
                {
                    calcmode = divide;

                }
                return;
            }
            /**/

            Value target = ParseAddress(label);
            target.SetBitness(bitness);
            
            std::string dest = target.Display();
            std::string src  = reg.UseValue();
            OutputAssign(dest, src);
        }
        void ClearBits(Register& reg, const labeldata& label, tristate bitness)
        {
            if(label.IsUnIndexedAddr())
            {
                output.modified_mem.insert(label.op1val);
            }

            Value target = ParseAddress(label);
            target.SetBitness(bitness);
            
            std::string dest = target.Display();
            std::string src  = reg.UseValue();
            OutputAssign(dest, src, "&= ~");
        }
        void SetBits(Register& reg, const labeldata& label, tristate bitness)
        {
            if(label.IsUnIndexedAddr())
            {
                output.modified_mem.insert(label.op1val);
            }

            Value target = ParseAddress(label);
            target.SetBitness(bitness);
            
            std::string dest = target.Display();
            std::string src  = reg.UseValue();
            OutputAssign(dest, src, "|=");
        }
        void StoreZero(const labeldata& label, tristate bitness)
        {
            if(label.IsUnIndexedAddr())
            {
                Value tmp;
                tmp.Set(0);
                memory[label.op1val] = tmp;
                memory[label.op1val].SetBitness(bitness);
                return;
            }

            Value target = ParseAddress(label);
            target.SetBitness(bitness);
            
            OutputAssign(target.Display(), "0");
        }

        void DisplayRegister(Register& reg)
        {
            if(reg.defined && !reg.displayed)
            {
                /***/if(A.displayed && reg.value == A.value)reg.value.SetRegister(A);
                else if(X.displayed && reg.value == X.value)reg.value.SetRegister(X);
                else if(Y.displayed && reg.value == Y.value)reg.value.SetRegister(Y);
                else if(D.displayed && reg.value == D.value)reg.value.SetRegister(D);
                else if(B.displayed && reg.value == B.value)reg.value.SetRegister(B);

                std::string regname = GetRegisterName(reg);

                if(reg.value.is_16bit.is_true())
                    regname = "word(" + regname + ")";
                else if(reg.value.is_16bit.is_false())
                    regname = "byte(" + regname + ")";
       
                if(&reg == &A) output.A_modified = true;
                if(&reg == &X) output.X_modified = true;
                if(&reg == &Y) output.Y_modified = true;
                if(&reg == &D) output.D_modified = true;
                if(&reg == &B) output.B_modified = true;

                OutputAssign(regname, reg.Display());
            }
        }

        void Push(const Register& reg)
        {
            stack.push(reg.value);
        }
        void Pop(Register& reg, tristate bitness)
        {
            reg.Load(stack.top(), bitness);
            stack.pop();
        }
        
        void OutputIfEQ(bool positive)
        {
            if(compare.type == Compare::BIT)
            {
                if(positive) OutputIf("&"); else OutputIf("&", true);
            }
            else
                OutputIf(positive ? "=" : "<>");
        }
        void OutputIfCC(bool positive)
        {
            OutputIf(positive ? "<" : ">=");
        }
        void OutputIfPL(bool positive)
        {
            if(compare.type == Compare::AUTO)
            {
                compare.Op2.Set(compare.Op1->value.is_16bit.is_true() ? 0x8000 : 0x80);
                compare.type = Compare::BIT;
                OutputIfEQ(!positive);
            }
            else
                OutputIf(positive ? "pl" : "mi");
        }
        void OutputCall(const std::string& label, bool is_far)
        {
            DisplayIndent();
            printf("call %s (%s)\n", label.c_str(), is_far ? "far" : "near");
        }
        
        void OutputAssign(const std::string& dest, const std::string& src,
                          const std::string& op = "=",
                          const std::string& prefix = "")
        {
            DisplayIndent();
            
            std::string destprefix = dest.substr(0,5);
            std::string srcprefix = src.substr(0,5);
            
            if(destprefix == "byte(" || destprefix == "word(") destprefix.erase(4);
            if(srcprefix == "byte(" || srcprefix == "word(") srcprefix.erase(4);
            
            unsigned src_ofs = 0;
            unsigned dest_ofs = 0;
            std::string suffix;

            if(destprefix == "byte")
            {
                if(srcprefix == "byte")
                    dest_ofs = 5, src_ofs = 5, suffix = "byte";
                else if(srcprefix == "word")
                    dest_ofs = 5, src_ofs = 5, suffix = "byte from word";
                else
                    dest_ofs = 5, src_ofs = 0, suffix = "byte";
            }
            else if(destprefix == "word")
            {
                if(srcprefix == "byte")
                    dest_ofs = 5, src_ofs = 5, suffix = "word from byte";
                else if(srcprefix == "word")
                    dest_ofs = 5, src_ofs = 5, suffix = "word";
                else
                    dest_ofs = 5, src_ofs = 0, suffix = "word";
            }
            
            string d = dest, s = src;
            if(dest_ofs) d = dest.substr(dest_ofs, dest.size()-dest_ofs-1);
            if(src_ofs) s = src.substr(src_ofs, src.size()-src_ofs-1);

            printf("%s%s %s %s", prefix.c_str(), d.c_str(), op.c_str(), s.c_str());
            if(!suffix.empty())printf(" (%s)", suffix.c_str());
            printf("\n");
        }
        void OutputIf(const std::string& optype, bool negate=false)
        {
            std::string op1 = compare.Op1->Display();
            std::string op2 = "?";
            
            switch(compare.type)
            {
                case Compare::CMP:
                case Compare::BIT:
                    op2 = compare.Op2.Display();
                    break;
                case Compare::AUTO:
                    op2 = "0";
                    break;
            }
            std::string prefix = "if ";
            if(negate) prefix += "not ";
            
            OutputAssign(op1, op2, optype, prefix);
            Indent(2);
        }
        
        void DisplayRegs()
        {
            // First display mem - it might also display registers.
            for(memorymap::iterator i = memory.begin(); i != memory.end(); ++i)
                DisplayMem(i);
            memory.clear();
            // First make sure regs don't depend on each others
            UndependRegs(A);
            UndependRegs(X);
            UndependRegs(Y);
            UndependRegs(D);
            UndependRegs(B);
            // Then display them
            DisplayRegister(A); if(compare.Op1 == &A) A.Undefine();
            DisplayRegister(X); if(compare.Op1 == &X) X.Undefine();
            DisplayRegister(Y); if(compare.Op1 == &Y) Y.Undefine();
            DisplayRegister(D);
            DisplayRegister(B);
        }
        
        void DisplayModifiedRegs(const Output& instruction)
        {
           #if 0
            if(instruction.A_modified && compare.Op1 != &A)
            {
                printf(";A modified\n");/*UndependRegs(A);*/DisplayRegister(A);
            }
            if(instruction.X_modified && compare.Op1 != &X)
            {
                printf(";X modified\n");/*UndependRegs(A);*/DisplayRegister(X);
            }
            if(instruction.Y_modified && compare.Op1 != &Y)
            {
                printf(";Y modified\n");/*UndependRegs(A);*/DisplayRegister(Y);
            }
            if(instruction.D_modified)
            {
                printf(";D modified\n");/*UndependRegs(D);*/DisplayRegister(D);
            }
            if(instruction.B_modified)
            {
                printf(";B modified\n");/*UndependRegs(B);*/DisplayRegister(B);
            }
           #endif
            for(std::set<unsigned>::const_iterator
                i = instruction.modified_mem.begin();
                i != instruction.modified_mem.end();
                ++i)
            {
                printf(";mem %X modified\n", *i);
                memorymap::iterator j = memory.find(*i);
                if(j != memory.end())
                    { DisplayMem(j); memory.erase(j); }
                else
                    printf(";but not found\n");
            }
        }
        
        void DisplayMem(memorymap::iterator i)
        {
            Value target;
            unsigned addr = i->first;
            if(addr < 0x100) target.SetL(addr, Value::Zero);
            else if(addr < 0x10000) target.SetL(addr, Value::Data);
            else target.SetL(addr, Value::Abs);
            OutputAssign(target.Display(), i->second.Display());
            
            output.modified_mem.insert(addr);
        }
        
        void UndependRegs(const Register& reg)
        {
            A.DontDepend(reg);
            X.DontDepend(reg);
            Y.DontDepend(reg);
            D.DontDepend(reg);
            B.DontDepend(reg);
            for(memorymap::iterator j,i = memory.begin(); i != memory.end(); i=j)
            {
                j=i;++j;
                if(i->second.Depends(reg))
                {
                    printf(";mem %X undepending because of %s\n",
                        i->first, reg.GetName().c_str());
                    DisplayMem(i);
                    memory.erase(i);
                }
            }
        }
        
        void UndefineRegisters()
        {
            A.Undefine(); output.A_modified = true;
            X.Undefine(); output.X_modified = true;
            Y.Undefine(); output.Y_modified = true;
            D.Undefine(); output.D_modified = true;
            B.Undefine(); output.B_modified = true;
            //carry = maybe;
        }
        
        const std::string GetRegisterName(const Register& reg)
        {
            if(&reg == &A) return "A";
            if(&reg == &X) return "X";
            if(&reg == &Y) return "Y";
            if(&reg == &D) return "D";
            if(&reg == &B) return "B";
            return "??";
        }
        
        void OutputRTS()
        {
            DisplayIndent();
            if(carry.is_true())
                printf("sec:rts\n");
            else if(carry.is_false())
                printf("clc:rts\n");
            else
                printf("rts\n");
        }
        
        void DisplayIndent()
        {
            int ind = indent;
            if(ind < 0)ind = 0;
            if(ind > 100)ind = 100;
            printf("\t%*s", ind, "");
        }

        void OutputEndIf()
        {
            Indent(-2);
            DisplayIndent();
            printf("end if\n");
        }
        void OutputElse()
        {
            Indent(-2);
            DisplayIndent();
            printf("else\n");
            Indent(2);
        }
        
        void CombineOutput(const Output& b)
        {
            if(b.A_modified) { output.A_modified = true; A.Undefine(); }
            if(b.X_modified) { output.X_modified = true; X.Undefine(); }
            if(b.Y_modified) { output.Y_modified = true; Y.Undefine(); }
            if(b.D_modified) { output.D_modified = true; D.Undefine(); }
            if(b.B_modified) { output.B_modified = true; B.Undefine(); }
        }
        
        void FinishIf(const Machine& tempmachine)
        {
            output.code += tempmachine.output.code;
            
            OutputEndIf();
            
            if(!tempmachine.output.terminated)
            {
                CombineOutput(tempmachine.output);
                // FIXME: modify "compare" too
            
                // If carry is not known the same in both
                if(!(carry != tempmachine.carry).is_false()) carry = maybe;
            }
            
            output.labels_seen.insert(tempmachine.output.labels_seen.begin(),
                                      tempmachine.output.labels_seen.end());
        }
        void FinishIf(const Machine& tempmachine, const Machine& tempmachine2)
        {
            output.code += tempmachine.output.code;
            
            OutputElse();
            
            output.code += tempmachine2.output.code;
            
            OutputEndIf();
            
            bool saved_dummy = output.dummy;
            const std::string saved_code = output.code;
            output = tempmachine.output;
            output.dummy = saved_dummy;
            output.code = saved_code;
            CombineOutput(tempmachine2.output);
            output.terminated = output.terminated && tempmachine2.output.terminated;
            
            if(!output.terminated)
            {
                // FIXME: modify "compare" too
            
                // If carry is not known the same in both
                if(!(carry != tempmachine.carry).is_false()) carry = maybe;
            }
            
            output.labels_seen.insert(tempmachine2.output.labels_seen.begin(),
                                      tempmachine2.output.labels_seen.end());
        }
        
        void OutputAsm(unsigned addr, const labeldata& label)
        {
            std::string code = label.op;
            if(!label.param1.empty())
            {
                code += ' ';
                code += label.param1;
            }
            if(!label.param2.empty())
            {
                code += ", ";
                code += label.param2;
            }
            
            DisplayIndent();
            printf("; %06X %s\n", addr, code.c_str());
        }
        
        Value ParseAddress(const labeldata& label)
        {
            Value result;

            if(label.IsUnIndexedAddr())
            {
                memorymap::const_iterator i = memory.find(label.op1val);
                if(i != memory.end())
                {
                    return i->second;
                }
                if(label.op1val == 0x4216)
                {
                    if(calcmode == multiply)
                    {
                        memorymap::iterator p1 = memory.find(0x4202);
                        memorymap::iterator p2 = memory.find(0x4203);
                        bool both = p1 != memory.end() && p2 != memory.end();
                        if(both)
                        {
                            result = p1->second;
                            result.Mul(p2->second);
                            result.SetBitness(true);
                        }
                        if(p1 != memory.end()) { if(!both) DisplayMem(p1); memory.erase(p1); }
                        if(p2 != memory.end()) { if(!both) DisplayMem(p2); memory.erase(p2); }
                        if(both) return result;
                    }
                    else
                    {
                        memorymap::iterator p1 = memory.find(0x4204);
                        memorymap::iterator p2 = memory.find(0x4205);
                    
                        bool both = p1 != memory.end() && p2 != memory.end();
                        if(both)
                        {
                            result = p1->second;
                            result.Div(p2->second);
                            result.SetBitness(true);
                        }
                        if(p1 != memory.end()) { if(!both) DisplayMem(p1); memory.erase(p1); }
                        if(p2 != memory.end()) { if(!both) DisplayMem(p2); memory.erase(p2); }
                        if(both) return result;
                    }
                }
                if(label.op1val == 0x4214)
                {
                    memorymap::iterator p1 = memory.find(0x4204);
                    memorymap::iterator p2 = memory.find(0x4205);
                    
                    bool both = p1 != memory.end() && p2 != memory.end();
                    if(both)
                    {
                        result = p1->second;
                        result.Mod(p2->second);
                        result.SetBitness(true);
                    }
                    if(p1 != memory.end()) { if(!both) DisplayMem(p1); memory.erase(p1); }
                    if(p2 != memory.end()) { if(!both) DisplayMem(p2); memory.erase(p2); }
                    if(both) return result;
                }
            }
            if(label.addrmode == 6 || label.addrmode == 14 || label.addrmode == 17)
            {
                memorymap::iterator p = memory.find(label.op1val);
                if(p != memory.end())
                {
                    DisplayMem(p);
                    memory.erase(p);
                }
            }
            
            switch(label.addrmode)
            {
                case 1: result.Set(label.op1val); break;
                case 2: result.Set(label.op1val); break;
                case 3: result.Set(label.op1val); break;
                // 4=rel8
                // 5=rel16
                case 6: result.SetL(label.op1val, Value::Zero); break;
                case 7: result.SetRL(label.op1val, Value::Zero, X); break;
                case 8: result.SetRL(label.op1val, Value::Zero, Y); break;
                case 9: result.SetLL(label.op1val, Value::Zero); break;
                case 10: result.SetRLL(label.op1val, Value::Zero, X); break;
                case 11: result.SetLRL(label.op1val, Value::Zero, Y); break;
                case 12: result.LSetLL(label.op1val, Value::Zero); break;
                case 13: result.LSetLRL(label.op1val, Value::Zero, Y); break;
                case 14: result.SetL(label.op1val, Value::Data); break;
                case 15: result.SetRL(label.op1val, Value::Data, X); break;
                case 16: result.SetRL(label.op1val, Value::Data, Y); break;
                case 17: result.SetL(label.op1val, Value::Abs); break;
                case 18: result.SetRL(label.op1val, Value::Abs, X); break;
                case 19: result.SetL(label.op1val, Value::StackZero); break;
                case 20: result.SetLRL(label.op1val, Value::StackZero, Y); break;
                case 21: result.SetLL(label.op1val, Value::Data); break;
                case 22: result.LSetLL(label.op1val, Value::Data); break;
                case 23: result.SetLRL(label.op1val, Value::Data, X); break;
                // 24=mvn
                case 25: result.Set(label.op1val); break;
            }
            /*char Buf[64];
            std::sprintf(Buf, "??addrmode_%u", label.addrmode);
            result.Set(Buf);*/
            return result;
        }
        
        void printf(const char* fmt, ...)
        {
            if(output.dummy) return;
            
            char Buf[4096];
            va_list ap;
            va_start(ap, fmt);
            std::vsnprintf(Buf, sizeof(Buf), fmt, ap);
            va_end(ap);
            output.code += Buf;
            
            //std::fprintf(stderr, "%s", Buf);
        }
        
    private:
        void Indent(int n) const { indent += n; }
    };
    
    void LoadCode(Machine& machine,
                  const unsigned first_address,
                  unsigned terminate_at=0)
    {
        static unsigned ind=0;
        std::fprintf(stderr, "%*s", ind++, "");
        std::fprintf(stderr, "LoadCode($%06X, $%06X, dummy=%s)\n",
            first_address, terminate_at,
            machine.output.dummy?"true":"false");
        
        unsigned address = first_address;
        for(;;)
        {
            std::map<unsigned, labeldata>::const_iterator i = labels.find(address);
            if(i == labels.end())
            {
                machine.DisplayRegs();
                machine.UndefineRegisters();
                machine.DisplayIndent();
                machine.printf("goto $%X\n", address);
                machine.output.terminated = true;
                break;
            }
            const labeldata& label = i->second;
            
            if(address == terminate_at
            || machine.output.labels_seen.find(address)
            != machine.output.labels_seen.end())
            {
                machine.DisplayRegs();
                //machine.OutputAsm(address, label);
                machine.DisplayIndent();
                machine.printf("--BREAK %X\n", address);
                break;
            }
            machine.output.labels_seen.insert(address);
            
            unsigned next_addr = label.nextlabel;

            if(!label.name.empty())
            {
                machine.printf("%s:\t;$%X\n", label.name.c_str(), address);
            }
            
            machine.OutputAsm(address, label);
            
            /***/if(label.op == "txa") { machine.A.Load(machine.X, label.A_16bit); }
            else if(label.op == "tya") { machine.A.Load(machine.Y, label.A_16bit); }
            else if(label.op == "tdc") { machine.A.Load(machine.D, true); }
            else if(label.op == "tax") { machine.X.Load(machine.A, label.X_16bit); }
            else if(label.op == "tyx") { machine.X.Load(machine.Y, label.X_16bit); }
            else if(label.op == "tay") { machine.Y.Load(machine.A, label.X_16bit); }
            else if(label.op == "txy") { machine.Y.Load(machine.X, label.X_16bit); }
            else if(label.op == "tcd") { machine.D.Load(machine.A, true); }
            else if(label.op == "lda") { machine.A.Load(label, label.A_16bit); }
            else if(label.op == "ldx") { machine.X.Load(label, label.X_16bit); }
            else if(label.op == "ldy") { machine.Y.Load(label, label.X_16bit); }
            else if(label.op == "pha") { machine.Push(machine.A); }
            else if(label.op == "phx") { machine.Push(machine.X); }
            else if(label.op == "phy") { machine.Push(machine.Y); }
            else if(label.op == "phd") { machine.Push(machine.D); }
            else if(label.op == "phb") { machine.Push(machine.B); }
            else if(label.op == "pla") { machine.Pop(machine.A, label.A_16bit); }
            else if(label.op == "plx") { machine.Pop(machine.X, label.X_16bit); }
            else if(label.op == "ply") { machine.Pop(machine.Y, label.X_16bit); }
            else if(label.op == "pld") { machine.Pop(machine.D, true); }
            else if(label.op == "plb") { machine.Pop(machine.B, false); }
            else if(label.op == "cmp") { machine.A.Compare(label, label.A_16bit); }
            else if(label.op == "cpx") { machine.X.Compare(label, label.X_16bit); }
            else if(label.op == "cpy") { machine.Y.Compare(label, label.X_16bit); }
            else if(label.op == "sta") { machine.Store(machine.A, label, label.A_16bit); }
            else if(label.op == "stx") { machine.Store(machine.X, label, label.X_16bit); }
            else if(label.op == "sty") { machine.Store(machine.Y, label, label.X_16bit); }
            else if(label.op == "trb") { machine.ClearBits(machine.A, label, label.A_16bit); }
            else if(label.op == "tsb") { machine.SetBits(machine.A, label, label.A_16bit); }
            else if(label.op == "stz") { machine.StoreZero(label, label.A_16bit); }

            else if(label.op == "inc") { machine.A.Add(1); }
            else if(label.op == "inx") { machine.X.Add(1); }
            else if(label.op == "iny") { machine.Y.Add(1); }
            else if(label.op == "dec") { machine.A.Sub(1); }
            else if(label.op == "dex") { machine.X.Sub(1); }
            else if(label.op == "dey") { machine.Y.Sub(1); }
            else if(label.op == "and") { machine.A.And(label, label.A_16bit); }
            else if(label.op == "ora") { machine.A.Or(label, label.A_16bit); }
            else if(label.op == "bit") { machine.A.Bit(label, label.A_16bit); }
            else if(label.op == "adc" && machine.carry.is_false()) { machine.A.Add(label, label.A_16bit); }
            else if(label.op == "sbc" && machine.carry.is_true()) { machine.A.Sub(label, label.A_16bit); }
            else if(label.op == "rts")
            {
                machine.DisplayRegs();
                machine.UndefineRegisters();
                machine.OutputRTS();
                machine.output.terminated = true;
                break;
            }
            else if(label.op == "clc")
            {
                machine.carry = false;
            }
            else if(label.op == "sec")
            {
                machine.carry = true;
            }
            else if(label.op == "xba"
                 || label.op == "adc"
                 || label.op == "asl"
                 || label.op == "eor"
                 || label.op == "rol"
                 || label.op == "ror"
                 || label.op == "sbc"
                 || label.op == "tsc"
                 || label.op == "xce")
            {
                machine.DisplayRegister(machine.A);
                machine.A.Undefine();
                machine.OutputAsm(address, label);
                machine.carry = maybe;
            }
            else if(label.op == "jsr"
                 || label.op == "jsl")
            {
                machine.DisplayRegs();
                machine.UndefineRegisters();
                
                std::map<unsigned, labeldata>::const_iterator j;
                
                j = labels.find(label.op1val);
                if(j == labels.end() || j->second.name.empty())
                {
                    machine.OutputAsm(address, label);
                }
                else
                {
                    machine.OutputCall(j->second.name, label.op=="jsl");
                }
            }
            else if(label.op == "sep" || label.op == "rep")
            {
                unsigned p = label.op1val;
                if(p & 1) { machine.carry = (label.op == "sep"); }
                p &= ~0x31;
                if(p) machine.OutputAsm(address, label);
            }
            else if(label.op == "beq"
                 || label.op == "bne"
                 || label.op == "bcc"
                 || label.op == "bcs"
                 || label.op == "bmi"
                 || label.op == "bpl")
            {
                if(machine.output.dummy)
                {
                    // Simplify so recursion won't take eternities
                    
                    machine.DisplayRegs();
                    machine.OutputIf("...");
                    
                    Machine tempmachine = machine;
                    tempmachine.output.Clear(true);
                    LoadCode(tempmachine, label.nextlabel, label.otherbranch);
                    machine.FinishIf(tempmachine);
                    
                    next_addr = label.otherbranch;
                }
                else
                {
                    bool positive = label.op == "beq"
                                 || label.op == "bcc"
                                 || label.op == "bpl";
                    
                    Machine tempmachine = machine; tempmachine.output.Clear(true);
                    Machine tempmachine2 = tempmachine;
                    
                    /* Step 1: Do a dry run to see which
                     * registers / memory addresses are handled
                     */
                    tempmachine.OutputIf("...");

                    unsigned iflabel    = label.nextlabel;
                    unsigned elselabel  = label.otherbranch;
                    unsigned endiflabel = label.otherbranch;
                    
                    LoadCode(tempmachine, iflabel, endiflabel);
                    LoadCode(tempmachine2, elselabel, terminate_at);
                    
                    std::set<unsigned> crossing;
                    std::set_intersection
                      ( tempmachine.output.labels_seen.begin(),
                        tempmachine.output.labels_seen.end(),
                        tempmachine2.output.labels_seen.begin(),
                        tempmachine2.output.labels_seen.end(),
                        std::inserter(crossing, crossing.begin())
                      );
                    if(!crossing.empty())
                    {
                        endiflabel  = *crossing.begin();
                        machine.printf("; endif=%X\n", endiflabel);
                        
                        // Revisit the code, because the ranges are now shorter.
                        tempmachine = machine; tempmachine.output.Clear(true);
                        tempmachine2 = tempmachine;
                        LoadCode(tempmachine, iflabel, endiflabel);
                        LoadCode(tempmachine2, elselabel, endiflabel);
                    }
                    else
                    {
                        // Else-part was not contiguous after this.
                        // Thus, clear it so it won't affect the
                        // DisplayModifiedRegs -call.
                        tempmachine2.output = Output();
                    }
                    
                    /* Step 2: Flush the registers which were
                     * seen as modified
                     */
                    machine.DisplayModifiedRegs(tempmachine.output);
                    machine.DisplayModifiedRegs(tempmachine2.output);
                    
                    if(label.nextlabel == endiflabel
                    || machine.output.labels_seen.find(label.nextlabel)
                    != machine.output.labels_seen.end())
                    {
                        // negate the jump because the "if" part was empty
                        positive = !positive;
                        unsigned tmp = iflabel;
                        iflabel = elselabel;
                        elselabel = tmp;
                    }
                    
                    /* Step 3: Do a real run with the branch */
                    if(label.op == "beq" || label.op == "bne")
                        machine.OutputIfEQ(!positive);
                    else if(label.op == "bcc" || label.op == "bcs")
                        machine.OutputIfCC(!positive);
                    else if(label.op == "bmi" || label.op == "bpl")
                        machine.OutputIfPL(!positive);
                    
                    tempmachine = machine;
                    tempmachine.output.Clear(false);
                    
                    LoadCode(tempmachine, iflabel, endiflabel);

                    if(endiflabel == elselabel)
                    {
                        machine.FinishIf(tempmachine);
                    }
                    else
                    {
                        tempmachine2 = machine;
                        tempmachine2.output.Clear(false);
                        tempmachine2.output.labels_seen = tempmachine.output.labels_seen;
                        
                        LoadCode(tempmachine2, elselabel, endiflabel);

                        machine.FinishIf(tempmachine, tempmachine2);
                    }
                    if(machine.output.terminated) break;
                    
                    /* Done */
                    next_addr = endiflabel;
                }
            }
            else
            {
                machine.OutputAsm(address, label);
            }

            address = next_addr;
        }
        machine.DisplayRegs();
        
        std::fprintf(stderr, "%*s", --ind, "");
        std::fprintf(stderr, "pop\n");
    }

    void DisplayRoutine(unsigned first_address, const std::string& name)
    {
        Machine machine;
        
        machine.Y.value.SetString("scriptpointer");
        machine.Y.value.SetBitness(true);
        machine.D.value.SetString("zeropage");
        machine.D.value.SetBitness(true);
        machine.B.value.SetString("datapage");
        machine.B.value.SetBitness(false);
        
        LoadCode(machine, first_address);
        
        machine.UndefineRegisters();
        
        std::fwrite(machine.output.code.data(), 1,
                    machine.output.code.size(),
                    stdout);
        std::fflush(stdout);
        
        /*
        for(std::map<unsigned, labeldata>::const_iterator
            i = labels.begin();
            i != labels.end();
            ++i)
        {
            const labeldata& label = i->second;
            
            bool X_modified = label.X_16bit != X_16bit;
            bool A_modified = label.A_16bit != A_16bit;
            
            if(!label.name.empty())
            {
                printf("%s:\n", label.name.c_str());
                A_modified = X_modified = true; // ensure they're specified
            }
            
            printf("%06X\t", i->first);
            
            if(X_modified) { X_16bit=label.X_16bit;  printf("%s: ", X_16bit?".xl":".xs"); }
            if(A_modified) { A_16bit=label.A_16bit;  printf("%s: ", A_16bit?".al":".as"); }
            
            
            printf("%s\n", code.c_str());

            if(label.barrier)
                printf("-----\n");
        }
        */
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
    
    void DisplayCode()
    {
        // Find a label.
        for(std::map<unsigned, labeldata>::const_iterator
            i = labels.begin();
            i != labels.end();
            ++i)
        {
            if(!i->second.name.empty())
            {
                DisplayRoutine(i->first, i->second.name);
                printf("----\n\n");
            }
        }
    }
}

static void LoadROM()
{
    static const char fn[] = "FIN/chrono-uncompressed.smc";
    FILE *fp = fopen(fn, "rb");
    if(!fp)
    {
        perror(fn);
        return;
    }
    LoadROM(fp);
    fclose(fp);
}

int main(void)
{
    LoadROM();
    
    DefinePointers();
    
    DisAssemble();
    
    FixJumps();
    
    DisplayCode();
    
    return 0;
}

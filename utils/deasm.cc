#include <cstdio>
#include <string>
#include <stack>
#include <cctype>
#include <cstdarg>
#include <algorithm>
#include <list>
#include <set>
#include <map>

#include "insdata.hh"
#include "rommap.hh"
#include "autoptr"

#include "deasm-disasm.hh"
#include "deasm-expr.hh"

//dummy, used by rommap.o
void PutAscii(const std::wstring& ) {}
void BlockComment(const std::wstring& ) {}
void StartBlock(const std::wstring&, const std::wstring&, unsigned=0) {}
void StartBlock(const std::wstring&, const std::wstring& ) {}
void EndBlock() {}
std::FILE *GetLogFile(const char*, const char*){return NULL;}

bool A_16bit;
bool X_16bit;

#undef BIT

int Expr::IndentLevel = 8;

typedef std::list<autoptr<struct Expr> > ExprChain;

bool operator< (const ExprChain::const_iterator& a,
                const ExprChain::const_iterator& b)
{
    return *a < *b;
}

namespace
{
    enum addrmode { Zero, StackZero, Data, Abs };
    
    class ExprData
    {
        typedef std::map<unsigned, ExprChain::iterator> exprmap;
        exprmap addr_to_expr;
        
        void AddJumpFrom(const Expr* tmp, unsigned addr)
        {
            exprmap::iterator k = addr_to_expr.find(addr);
            if(k == addr_to_expr.end())
            {
                std::printf("--- No address: $%06X\n", addr);
            }
            else
                (*k->second)->jump_from.push_back(tmp);
        }
        
    public:
        ExprData(ExprChain& chain)
        {
            for(ExprChain::iterator
                j = chain.begin(); j != chain.end(); ++j)
            {
                const Expr* tmp = *j;
                if(tmp->has_address)
                {
                    unsigned addr = tmp->address;
                    
                    exprmap::iterator k = addr_to_expr.find(addr);
                    if(k == addr_to_expr.end())
                        addr_to_expr[tmp->address] = j;
                    else
                        std::printf("--- Duplicate label: $%06X\n", addr);
                }
                else
                {
                    /*
                    std::printf("--- No label:\n");
                    tmp->Dump();
                    std::printf("\n---\n");
                    */
                }
            }

            for(ExprChain::iterator
                j = chain.begin(); j != chain.end(); ++j)
            {
                const Expr* tmp = *j;
                
                if(tmp->type == Expr::Goto)
                    AddJumpFrom(tmp, tmp->intval);
                if(tmp->type == Expr::IfGoto)
                {
                    AddJumpFrom(tmp, tmp->intval);
                    AddJumpFrom(tmp, tmp->intval2);
                }
            }
        }
        
        void Redefine(ExprChain& chain,
                      ExprChain::iterator old_it,
                      ExprChain::iterator new_it)
        {
            for(exprmap::iterator
                i = addr_to_expr.begin();
                i != addr_to_expr.end();
                ++i)
            {
                if(i->second == old_it) i->second = new_it;
            }
            for(ExprChain::iterator j = chain.begin(); j != chain.end(); ++j)
            {
                std::list<const Expr*>& jump_from = (*j)->jump_from;
                for(std::list<const Expr*>::iterator
                    k = jump_from.begin(); k != jump_from.end(); ++k)
                {
                    if(*k == *old_it) *k = *new_it;
                }
            }
        }

        void DumpChain(ExprChain& a)
        {
            for(ExprChain::iterator
                j = a.begin(); j != a.end(); ++j)
            {
                Expr& exp = **j;
                
                exp.ShowIndent();
                exp.Dump();
                std::printf("\n");
                if(!exp.address)
                {
                    exp.ShowIndent();
                    std::printf("; No address\n");
                }
                if(exp.type == Expr::Goto
                || exp.type == Expr::IfGoto)
                {
                    if(addr_to_expr.find(exp.intval) == addr_to_expr.end())
                    {
                        exp.ShowIndent();
                        std::printf("; $%06X not found\n", exp.intval);
                    }
                }
                if(exp.type == Expr::IfGoto)
                {
                    if(addr_to_expr.find(exp.intval2) == addr_to_expr.end())
                    {
                        exp.ShowIndent();
                        std::printf("; $%06X not found\n", exp.intval2);
                    }
                }
            }
        }
    
        void TraceUsage(ExprChain& chain,
                        ExprChain::iterator referer_it,
                        ExprChain::iterator expr_it,
                        ReferenceRecord& rec,
                        Expr::ExprType type,
                        unsigned param,
                        bool first=true)
        {
            static std::set<ExprChain::iterator> seen;
            if(first) seen.clear();
            
            if(rec.IsRedefined()) return;
            for(; expr_it != chain.end(); referer_it=expr_it++)
            {
                if(seen.find(expr_it) != seen.end())
                {
                    rec.LoopDetected();
                    return;
                }
                seen.insert(expr_it);
                
                Expr& exp = **expr_it;
                
                if(!exp.jump_from.empty())
                {
                    rec.AssumeWorst();
                    return;
                }
                
                exp.TraceUsage(rec, type, param);
                if(rec.IsRedefined()) return;
                
                if(exp.type == Expr::Goto)
                {
                    exprmap::iterator i = addr_to_expr.find(exp.intval);
                    if(i != addr_to_expr.end())
                        TraceUsage(chain, expr_it, i->second, rec, type, param, false);
                    return;
                }
                if(exp.type == Expr::IfGoto)
                {
                    exprmap::iterator i;
                    if((i = addr_to_expr.find(exp.intval)) != addr_to_expr.end())
                        TraceUsage(chain, expr_it, i->second, rec, type, param, false);
                    else
                        rec.AssumeWorst();
                    
                    if((i = addr_to_expr.find(exp.intval2)) != addr_to_expr.end())
                    {
                        ReferenceRecord rec2(rec);
                        TraceUsage(chain, expr_it, i->second, rec2, type, param, false);

                        rec.Combine(rec2);
                    }
                    else
                        rec.AssumeWorst();
                }
                if(exp.IsTerminal())
                    return;
            }
        }
    };
    
    void DumpCode(ExprChain& chain)
    {
        ExprData data(chain);
        
        std::map<unsigned, exprp> reg_contents;
        std::map<unsigned, exprp> mem_direct;
        std::map<unsigned, exprp> mem_data;
        std::map<unsigned, exprp> mem_abs;
        
#if 0 /* ASSIGN INITIAL REGISTER CONTENTS? */
        reg_contents['Y'] = new Expr(Expr::NamedConstant, "scriptpointer");
        reg_contents['Y']->SetBitness(16);
        reg_contents['D'] = new Expr(Expr::NamedConstant, "zeropage");
        reg_contents['D']->SetBitness(16);
        reg_contents['B'] = new Expr(Expr::NamedConstant, "datapage");
        reg_contents['B']->SetBitness(8);
#endif
        
        for(ExprChain::iterator next,j = chain.begin(); j != chain.end(); j=next)
        {
            next = j; ++next;
            
#if 1
            /* This must be done before the assigns. */
            (*j)->Redefine(*j, Expr::Register, reg_contents);
            (*j)->Redefine(*j, Expr::DirectAddr, mem_direct);
            (*j)->Redefine(*j, Expr::DataAddr, mem_data);
            (*j)->Redefine(*j, Expr::AbsAddr, mem_abs);
#endif
            
            Expr& exp = **j;
            
            if(exp.type == Expr::Assign)
            {
                //Expr& p1 = *exp.p1;
                //Expr& p2 = *exp.p2;
                
                bool Eat = false;

#if 0 /* CHECK EATING? */
                switch(p1.type)
                {
                    case Expr::Register:
                    {
                        reg_contents[p1.intval] = &p2;

                        ReferenceRecord rec;
                        data.TraceUsage(chain,j,next, rec, p1.type, p1.intval);
                        
#if 0
                        printf("Assign to %c - redefined=%s, min=%d, max=%d\n",
                            p1.intval, rec.IsRedefined()?"true":"false",
                            rec.GetMinTimesUsed(),
                            rec.GetMaxTimesUsed());
#endif
                        
                        if(!rec.IsRedefined() || rec.GetMinTimesUsed() > 2)
                        {
                            // flush, forget
                            reg_contents.erase(p1.intval);
                        }
                        else
                        {
                            Eat = true;
                        }
                        
                        break;
                    }
                    case Expr::DirectAddr:
                    {
                        if(!p1.p1->IsConst()) continue;
                        unsigned addr = p1.p1->GetConst();
                        mem_direct[addr] = &p2;
                        
                        /* TODO:
                         *
                         *  W = true if it will be redefined
                         *  R1 = min. times it'll be read before redefined
                         *  R2 = max. times it'll be read before redefined
                         *
                         * - Always remember the value
                         *
                         * - If (W = false) or (R1 > 1)
                         *     - flush
                         *     - forget value
                         *
                         */
                        
                        ReferenceRecord rec;
                        data.TraceUsage(chain,j,next, rec, p1.type, addr);
                        
                        printf("Assign to [%02X] - redefined=%s, min=%d, max=%d\n",
                            addr, rec.IsRedefined()?"true":"false",
                            rec.GetMinTimesUsed(),
                            rec.GetMaxTimesUsed());
                        
                        if(!rec.IsRedefined() || rec.GetMinTimesUsed() > 1)
                        {
                            // flush, forget
                            mem_direct.erase(addr);
                        }
                        else
                        {
                            Eat = true;
                        }
                        
                        break;
                    }
                    case Expr::DataAddr:
                    {
                        if(!p1.p1->IsConst()) continue;
                        unsigned addr = p1.p1->GetConst();
                        mem_data[addr] = &p2;

                        ReferenceRecord rec;
                        data.TraceUsage(chain,j,next, rec, p1.type, addr);
                        
                        printf("Assign to [%04X] - redefined=%s, min=%d, max=%d\n",
                            addr, rec.IsRedefined()?"true":"false",
                            rec.GetMinTimesUsed(),
                            rec.GetMaxTimesUsed());
                        
                        if(!rec.IsRedefined() || rec.GetMinTimesUsed() > 1)
                        {
                            // flush, forget
                            mem_data.erase(addr);
                        }
                        else
                        {
                            Eat = true;
                        }
                        
                        break;
                    }
                    case Expr::AbsAddr:
                    {
                        if(!p1.p1->IsConst()) continue;
                        unsigned addr = p1.p1->GetConst();
                        mem_abs[addr] = &p2;

                        ReferenceRecord rec;
                        data.TraceUsage(chain,j,next, rec, p1.type, addr);
                        
                        printf("Assign to [%06X] - redefined=%s, min=%d, max=%d\n",
                            addr, rec.IsRedefined()?"true":"false",
                            rec.GetMinTimesUsed(),
                            rec.GetMaxTimesUsed());
                        
                        if(!rec.IsRedefined() || rec.GetMinTimesUsed() > 1)
                        {
                            // flush, forget
                            mem_abs.erase(addr);
                        }
                        else
                        {
                            Eat = true;
                        }

                        break;
                    }
                    default:
                        break;
                }
#endif
                
                if(Eat)
                {
                    data.Redefine(chain, j, next);
                    chain.erase(j);
                }
            }
        }
    
        data.DumpChain(chain);
    }
    
    class Compiler
    {
        struct Output
        {
            ExprChain chain;
            bool dummy;
            
            std::set<std::string> labels;
            bool address_given;
            unsigned address;
            
            Output(): dummy(false), address_given(false)
            {
            }
            
            void Clear()
            {
                chain.clear();
                address_given=false;
            }
            
            void AddExpr(Expr* expression, tristate bitness=maybe)
            {
                if(bitness.is_true()) expression->SetBitness(16);
                if(bitness.is_false()) expression->SetBitness(8);
                
                if(!labels.empty())
                {
                    expression->labels.insert(labels.begin(), labels.end());
                    labels.clear();
                }
                if(address_given)
                {
                    expression->SetAddress(address);
                    address_given = false;
                }
                
                chain.push_back(expression);
            }
            
            void SetAddress(unsigned addr)
            {
                address = addr;
                address_given = true;
            }
            
            void AddLabel(const std::string& name)
            {
                labels.insert(name);
            }
        } output;
        
        void AddExpr(Expr* expression, tristate bitness=maybe)
        {
            output.AddExpr(expression, bitness);
        }
        
        void OutputAddress(unsigned address)
        {
            output.SetAddress(address);
        }

        void OutputLabel(const std::string& name)
        {
            output.AddLabel(name);
        }

        
        void Store(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            Expr* target = ParseAddress(label);
            Expr* param  = new Expr(Expr::Register, regkey);
            AddExpr(new Expr(Expr::Assign, target, param),
                    bitness);
        }
        void ClearBits(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            Expr* target = ParseAddress(label);
            Expr* param  = new Expr(Expr::Register, regkey);
            AddExpr(new Expr(Expr::Assign, target, new Expr(Expr::AndNot, target, param)),
                    bitness);
        }
        void SetBits(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            Expr* target = ParseAddress(label);
            Expr* param  = new Expr(Expr::Register, regkey);
            AddExpr(new Expr(Expr::Assign, target, new Expr(Expr::Or, target, param)),
                    bitness);
        }
        void StoreZero(const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, ParseAddress(label),
                             new Expr(Expr::Constant, 0)),
                    bitness);
        }
        void Push(RegisterNameType regkey, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Push, new Expr(Expr::Register, regkey)),
                    bitness);
        }
        void Pop(RegisterNameType regkey, tristate bitness)
        {
            AddExpr(new Expr(Expr::Pop, new Expr(Expr::Register, regkey)),
                    bitness);
        }
        void LoadReg(RegisterNameType destregkey,
                     RegisterNameType srcregkey,
                     tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, destregkey),
                                           new Expr(Expr::Register, srcregkey)),
                    bitness);
        }
        void LoadReg(RegisterNameType destregkey,
                     const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, destregkey),
                                          ParseAddress(label)),
                    bitness);
        }
        void Add(RegisterNameType regkey, int val, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                       new Expr(Expr::Add, new Expr(Expr::Register, regkey), 
                                           new Expr(Expr::Constant, val))),
                    bitness);
        }
        void Sub(RegisterNameType regkey, int val, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                       new Expr(Expr::Sub, new Expr(Expr::Register, regkey), 
                                           new Expr(Expr::Constant, val))),
                    bitness);
        }
        void And(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                       new Expr(Expr::And, new Expr(Expr::Register, regkey), 
                                           ParseAddress(label))),
                    bitness);
        }
        void Or(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                        new Expr(Expr::Or, new Expr(Expr::Register, regkey), 
                                           ParseAddress(label))),
                    bitness);
        }
        void Xor(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                       new Expr(Expr::Xor, new Expr(Expr::Register, regkey), 
                                           ParseAddress(label))),
                    bitness);
        }
        void Adc(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                       new Expr(Expr::Adc, new Expr(Expr::Register, regkey), 
                                           ParseAddress(label))),
                    bitness);
        }
        void Add(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                       new Expr(Expr::Add, new Expr(Expr::Register, regkey), 
                                           ParseAddress(label))),
                    bitness);
        }
        void Sbc(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                       new Expr(Expr::Sbc, new Expr(Expr::Register, regkey), 
                                           ParseAddress(label))),
                    bitness);
        }
        void Sub(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Assign, new Expr(Expr::Register, regkey),
                       new Expr(Expr::Sub, new Expr(Expr::Register, regkey), 
                                           ParseAddress(label))),
                    bitness);
        }
        void Compare(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Cmp, new Expr(Expr::Register, regkey), 
                                        ParseAddress(label)),
                    bitness);
        }
        void Bit(RegisterNameType regkey, const labeldata& label, tristate bitness)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Bit, new Expr(Expr::Register, regkey), 
                                        ParseAddress(label)),
                    bitness);
        }
        
        void OutputCall(const std::string& label, bool is_far)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Call, label));
        }
        void OutputGoto(unsigned label)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Goto, label));
        }
        void Rts(tristate carry)
        {
            if(output.dummy)return;
            if(carry.is_true())
                AddExpr(new Expr(Expr::RtsSec));
            else if(carry.is_false())
                AddExpr(new Expr(Expr::RtsClc));
            else
                AddExpr(new Expr(Expr::Rts));
        }
        void Clc()
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Asm, "clc"));
        }
        void Sec()
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::Asm, "sec"));
        }
        void OutputAsm(const labeldata& label)
        {
            if(output.dummy)return;
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
            AddExpr(new Expr(Expr::Asm, code));
        }
        void OutputComment(const char *fmt, ...)
        {
            if(output.dummy)return;
            char Buf[4096];
            va_list ap;
            va_start(ap, fmt);
            std::vsnprintf(Buf, sizeof Buf, fmt, ap);
            va_end(ap);
            AddExpr(new Expr(Expr::Comment, Buf));
        }
        
        void OutputIfGoto(Expr* cond, unsigned addr_if, unsigned addr_else)
        {
            if(output.dummy)return;
            AddExpr(new Expr(Expr::IfGoto, cond, addr_if, addr_else));
        }
        
        static Expr* BuildAddr(Expr *addr, addrmode where)
        {
            switch(where)
            {
                case Zero: return new Expr(Expr::DirectAddr, addr);
                case StackZero: return new Expr(Expr::StackDirect, addr);
                case Data: return new Expr(Expr::DataAddr, addr);
                case Abs: return new Expr(Expr::AbsAddr, addr); break;
            }
            return NULL;
        }
        static Expr* SetL(unsigned addr, addrmode where) // [addr]
        {
            return BuildAddr(new Expr(Expr::Constant, addr), where);
        }
        static Expr* SetLL(unsigned addr, addrmode where) // [word[addr]]
        {
            Expr* a = BuildAddr(new Expr(Expr::Constant, addr), where);
            a->SetBitness(16);
            return new Expr(Expr::DataAddr, a);
        }
        static Expr* SetRL(unsigned addr, addrmode where,
                           RegisterNameType regkey) // [addr+reg]
        {
            Expr* a = new Expr(Expr::Add, new Expr(Expr::Constant, addr),
                                          new Expr(Expr::Register, regkey));
            return BuildAddr(a, where);
        }
        static Expr* SetRLL(unsigned addr, addrmode where,
                            RegisterNameType regkey) // [word[addr+reg]]
        {
            Expr* a = new Expr(Expr::Add, new Expr(Expr::Constant, addr),
                                          new Expr(Expr::Register, regkey));
            a = BuildAddr(a, where);
            a->SetBitness(16);
            return new Expr(Expr::DataAddr, a);
        }
        static Expr* SetLRL(unsigned addr, addrmode where,
                            RegisterNameType regkey) // [word[addr]+reg]
        {
            Expr* a = BuildAddr(new Expr(Expr::Constant, addr), where);
            a->SetBitness(16);
            return new Expr(Expr::DataAddr, new Expr(Expr::Add, a,
                                            new Expr(Expr::Register, regkey)));
        }
        static Expr* LSetLL(unsigned addr, addrmode where) // [long[addr]]
        {
            Expr* a = BuildAddr(new Expr(Expr::Constant, addr), where);
            a->SetBitness(24);
            return new Expr(Expr::DataAddr, a);
        }
        static Expr* LSetLRL(unsigned addr, addrmode where,
                             RegisterNameType regkey) // [long[addr]+reg]
        {
            Expr* a = new Expr(Expr::Add, new Expr(Expr::Constant, addr),
                                          new Expr(Expr::Register, regkey));
            a = BuildAddr(a, where);
            a->SetBitness(24);
            return new Expr(Expr::DataAddr, a);
        }
        
        static Expr* ParseAddress(const labeldata& label)
        {
            switch(label.addrmode)
            {
                case 1:
                case 2:
                case 3:
                case 25: return new Expr(Expr::Constant, label.op1val);
                // 4=rel8
                // 5=rel16
                case 6: return SetL(label.op1val, Zero); break;
                case 7: return SetRL(label.op1val, Zero, 'X'); break;
                case 8: return SetRL(label.op1val, Zero, 'Y'); break;
                case 9: return SetLL(label.op1val, Zero); break;
                case 10: return SetRLL(label.op1val, Zero, 'X'); break;
                case 11: return SetLRL(label.op1val, Zero, 'Y'); break;
                case 12: return LSetLL(label.op1val, Zero); break;
                case 13: return LSetLRL(label.op1val, Zero, 'Y'); break;
                case 14: return SetL(label.op1val, Data); break;
                case 15: return SetRL(label.op1val, Data, 'X'); break;
                case 16: return SetRL(label.op1val, Data, 'Y'); break;
                case 17: return SetL(label.op1val, Abs); break;
                case 18: return SetRL(label.op1val, Abs, 'X'); break;
                case 19: return SetL(label.op1val, StackZero); break;
                case 20: return SetLRL(label.op1val, StackZero, 'Y'); break;
                case 21: return SetLL(label.op1val, Data); break;
                case 22: return LSetLL(label.op1val, Data); break;
                case 23: return SetLRL(label.op1val, Data, 'X'); break;
                // 24=mvn
            }
            std::printf(
                "ParseAddress: addrmode %u is unhandled\n"
                "In %s %s %s\n",
                    label.addrmode,
                    label.op.c_str(),
                    label.param1.c_str(),
                    label.param2.c_str());
            return NULL;
        }
        
    public:
        Compiler()
        {
        }

        std::set<unsigned> labels_seen;
        
        struct BranchData
        {
            unsigned addr;
            tristate carry;
            
            BranchData(): addr(0), carry(maybe) {}
        };
        const ExprChain LoadCode
           (const unsigned first_address,
            unsigned terminate_at=0,
            tristate carry=maybe)
        {
            std::list<BranchData> do_last;
            
            unsigned prev_address = 0;
            unsigned address = first_address;
            bool carry_manual = false;
            for(;;)
            {
                std::map<unsigned, labeldata>::const_iterator i = labels.find(address);
                if(i == labels.end())
                {
                    if(address == first_address)
                    {
                        //fprintf(stderr, "LoadCode: %06X not found\n", first_address);
                    }
                    else
                    {
                        //OutputAddress(prev_address); //useless?
                        OutputGoto(address);
                    }
                    break;
                }
                const labeldata& label = i->second;
                
                if(address == terminate_at
                || labels_seen.find(address) != labels_seen.end())
                {
                    //OutputComment("BREAK %X", address);
                    OutputAddress(prev_address);
                    OutputGoto(address);
                    break;
                }

                OutputAddress(address);

                labels_seen.insert(address);
                
                unsigned next_addr = label.nextlabel;

                if(!label.name.empty())
                {
                    OutputLabel(label.name);
                }
                
                //OutputAsm(label);
                
                tristate new_carry = maybe;
                
                // Carry is affected by:
                //    adc, sbc
                //    asl, lsr
                //    cmp, cpx, cpy
                //    rol, ror
                //    clc, sec, xce, (rep), (sep)
                // Carry is read by:
                //    adc, sbc
                //    rol, ror
                //    bcc, bcs, xce
                //    - and in our case, rts
                
                if(!carry.is_maybe()
                && carry_manual
                && label.op != "adc" && label.op != "sbc"
                && label.op != "rts")
                {
                    if(carry) Sec(); else Clc();
                }
                
                carry_manual = false;
                
                if(label.op != "adc" && label.op != "sbc"
                && label.op != "asl" && label.op != "lsr"
                && label.op != "cmp" && label.op != "cpx" && label.op != "cpy"
                && label.op != "rol" && label.op != "ror"
                && label.op != "xce" && label.op != "rep" && label.op != "sep")
                {
                    new_carry = carry;
                }
                
                /***/if(label.op == "txa") { LoadReg('A', 'X', label.A_16bit); }
                else if(label.op == "tya") { LoadReg('A', 'Y', label.A_16bit); }
                else if(label.op == "tdc") { LoadReg('A', 'D', true); }
                else if(label.op == "tax") { LoadReg('X', 'A', label.X_16bit); }
                else if(label.op == "tyx") { LoadReg('X', 'Y', label.X_16bit); }
                else if(label.op == "tay") { LoadReg('Y', 'A', label.X_16bit); }
                else if(label.op == "txy") { LoadReg('Y', 'X', label.X_16bit); }
                else if(label.op == "tcd") { LoadReg('D', 'A', true); }
                else if(label.op == "lda") { LoadReg('A', label, label.A_16bit); }
                else if(label.op == "ldx") { LoadReg('X', label, label.X_16bit); }
                else if(label.op == "ldy") { LoadReg('Y', label, label.X_16bit); }
                else if(label.op == "pha") { Push('A', label.A_16bit); }
                else if(label.op == "phx") { Push('X', label.X_16bit); }
                else if(label.op == "phy") { Push('Y', label.X_16bit); }
                else if(label.op == "phd") { Push('D', true);  }
                else if(label.op == "phb") { Push('B', false); }
                else if(label.op == "pla") { Pop('A', label.A_16bit); }
                else if(label.op == "plx") { Pop('X', label.X_16bit); }
                else if(label.op == "ply") { Pop('Y', label.X_16bit); }
                else if(label.op == "pld") { Pop('D', true); }
                else if(label.op == "plb") { Pop('B', false); }
                else if(label.op == "cmp") { Compare('A', label, label.A_16bit); }
                else if(label.op == "cpx") { Compare('X', label, label.X_16bit); }
                else if(label.op == "cpy") { Compare('Y', label, label.X_16bit); }
                else if(label.op == "sta") { Store('A', label, label.A_16bit); }
                else if(label.op == "stx") { Store('X', label, label.X_16bit); }
                else if(label.op == "sty") { Store('Y', label, label.X_16bit); }
                else if(label.op == "trb") { ClearBits('A', label, label.A_16bit); }
                else if(label.op == "tsb") { SetBits('A', label, label.A_16bit); }
                else if(label.op == "stz") { StoreZero(label, label.A_16bit); }

                else if(label.op == "inc") { Add('A', 1, label.A_16bit); }
                else if(label.op == "inx") { Add('X', 1, label.X_16bit); }
                else if(label.op == "iny") { Add('Y', 1, label.X_16bit); }
                else if(label.op == "dec") { Sub('A', 1, label.A_16bit); }
                else if(label.op == "dex") { Sub('X', 1, label.X_16bit); }
                else if(label.op == "dey") { Sub('Y', 1, label.X_16bit); }
                else if(label.op == "and") { And('A', label, label.A_16bit); }
                else if(label.op == "ora") { Or('A', label, label.A_16bit); }
                else if(label.op == "eor") { Xor('A', label, label.A_16bit); }
                else if(label.op == "bit") { Bit('A', label, label.A_16bit); }
                else if(label.op == "adc")
                {
                    if(carry.is_false())
                        Add('A', label, label.A_16bit);
                    else if(carry.is_true())
                    {
                        Add('A', label, label.A_16bit);
                        Add('A', 1, label.A_16bit);
                    }
                    else
                        Adc('A', label, label.A_16bit);
                }
                else if(label.op == "sbc")
                {
                    if(carry.is_true())
                        Sub('A', label, label.A_16bit);
                    else if(carry.is_false())
                    {
                        Sub('A', label, label.A_16bit);
                        Sub('A', 1, label.A_16bit);
                    }
                    else
                        Sbc('A', label, label.A_16bit);
                }
                else if(label.op == "clc")
                {
                    new_carry = false;
                    carry_manual = true;
                    if(label.referers.size() > 1) OutputAsm(label);
                }
                else if(label.op == "sec")
                {
                    new_carry = true;
                    carry_manual = true;
                    if(label.referers.size() > 1) OutputAsm(label);
                }
                else if(label.op == "rts")
                {
                    Rts(carry);
                    carry_manual = false;
                    break;
                }
                else if(label.op == "jsr"
                     || label.op == "jsl")
                {
                    std::map<unsigned, labeldata>::const_iterator j;
                    
                    j = labels.find(label.op1val);
                    if(j == labels.end() || j->second.name.empty())
                    {
                        OutputAsm(label);
                    }
                    else
                    {
                        OutputCall(j->second.name, label.op=="jsl");
                    }
                }
                else if(label.op == "sep" || label.op == "rep")
                {
                    OutputAsm(label);
                }
                else if(label.op == "beq" || label.op == "bne")
                {
                    OutputIfGoto(new Expr(Expr::ZeroTest, label.op == "beq"),
                                 label.otherbranch, next_addr);
                    goto branch;
                }
                else if(label.op == "bcs" || label.op == "bcc")
                {
                    new_carry = label.op == "bcs";
                    carry     = label.op == "bcc";
                    OutputIfGoto(new Expr(Expr::CarryTest, label.op == "bcs"),
                                 label.otherbranch, next_addr);
                    goto branch;
                }
                else if(label.op == "bmi" || label.op == "bpl")
                {
                    OutputIfGoto(new Expr(Expr::SignTest, label.op == "bmi"),
                                 label.otherbranch, next_addr);
                    goto branch;
                }
                else if(label.op == "bvs" || label.op == "bvc")
                {
                    OutputIfGoto(new Expr(Expr::OverflowTest, label.op == "bvs"),
                                 label.otherbranch, next_addr);
                branch:
                    BranchData tmp;
                    tmp.addr  = label.otherbranch;
                    tmp.carry = new_carry;
                    do_last.push_back(tmp);
                    tmp.addr  = next_addr;
                    tmp.carry = carry;
                    do_last.push_back(tmp);
                    carry_manual = false;
                    break;
                }
                else
                {
                    OutputAsm(label);
                }
                
                carry = new_carry;
                
                prev_address = address;
                address = next_addr;
            }
            
            if(!carry.is_maybe() && carry_manual) { if(carry) Sec(); else Clc(); }
            
            ExprChain result = output.chain;
            output.Clear();
            
            for(std::list<BranchData>::const_iterator 
                i = do_last.begin(); i != do_last.end(); ++i)
            {
                if(labels_seen.find(i->addr) != labels_seen.end())
                {
                    //fprintf(stderr, "$%06X already seen.\n", i->addr);
                    continue;
                }
                //fprintf(stderr, "Visiting $%06X.\n", i->addr);
                
                const ExprChain more = LoadCode(i->addr, terminate_at, i->carry);
                
                result.insert(result.end(), more.begin(), more.end());
                //std::copy(more.begin(), more.end(), std::back_inserter(result));
            }
            return result;
        }
    };
    
    void DisplayRoutine(unsigned first_address, const std::string& name)
    {
        Compiler machine;
        
 /*
        'Y'.value.SetString("scriptpointer");
        'Y'.value.SetBitness(true);
        'D'.value.SetString("zeropage");
        'D'.value.SetBitness(true);
        machine.B.value.SetString("datapage");
        machine.B.value.SetBitness(false);
*/
        
        ExprChain code = machine.LoadCode(first_address);
        DumpCode(code);
        
/*
        std::fwrite(machine.output.code.data(), 1,
                    machine.output.code.size(),
                    stdout);
        std::fflush(stdout);
*/

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
                printf("----routine----\n\n");
                DisplayRoutine(i->first, i->second.name);
                printf("----end routine----\n\n");
            }
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
    
    FixJumps(); // Jump to jump -> jump
    FixReps();  // Find REP/SEP and convert them.
    
    DisplayCode();
    
    return 0;
}

#ifndef bqtDeasmExprHH
#define bqtDeasmExprHH

#include <cstdio>
#include <string>
#include <list>
#include <set>

#include "autoptr"
#include "utils/deasm-disasm.hh"

typedef unsigned RegisterNameType;

class ReferenceRecord
{
    bool MightBeRedefined;
    int MinTimesUsed;
    int MaxTimesUsed;

public:
    ReferenceRecord(): MightBeRedefined(false), MinTimesUsed(0), MaxTimesUsed(0) { }
    
    void SetRedefined(bool v=true) { MightBeRedefined=v; }
    void AddUsed(int n=1) { MinTimesUsed += n; MaxTimesUsed += n; }
    
    bool IsRedefined() const { return MightBeRedefined; }
    int GetMinTimesUsed() const { return MinTimesUsed; }
    int GetMaxTimesUsed() const { return MaxTimesUsed; }
    
    void Combine(const ReferenceRecord& b)
    {
        if(b.MightBeRedefined) MightBeRedefined = true;
        
        if(b.MinTimesUsed < MinTimesUsed) MinTimesUsed = b.MinTimesUsed;
        if(b.MaxTimesUsed > MaxTimesUsed) MaxTimesUsed = b.MaxTimesUsed;
    }
    
    void AssumeWorst() // Used when we don't know what happens.
    {
        AddUsed(1000);
        SetRedefined();
    }
    
    void LoopDetected()
    {
        if(MaxTimesUsed > 0) MaxTimesUsed += 1000;
    }
};

struct Expr: public ptrable
{
    enum ExprType
    {
        Constant,           //intval
        NamedConstant,      //strval
        Register,           //intval
        DirectAddr,         //p1
        StackDirect,        //p1
        DataAddr,           //p1
        AbsAddr,            //p1
        Add,                //p1,p2
        Sub,                //p1,p2
        Adc,                //p1,p2
        Sbc,                //p1,p2
        Or,                 //p1,p2
        And,                //p1,p2
        AndNot,             //p1,p2
        Xor,                //p1,p2
        Div,                //p1,p2
        Mod,                //p1,p2
        Mul,                //p1,p2
        Cmp,                //p1,p2
        Bit,                //p1,p2
        Assign,             //p1,p2
        Push,               //p1
        Pop,                //p1
        RtsClc,
        RtsSec,
        Rts,
        Asm,                //strval
        Comment,            //strval
        Call,               //strval
        Goto,               //intval
        CarryTest,          //boolval
        ZeroTest,           //boolval
        SignTest,           //boolval
        OverflowTest,       //boolval
        IfGoto              //p1,intval,intval2
    } type;
    
    int bits;
    
    std::set<std::string> labels;
    unsigned address; bool has_address;
    
    std::list<const Expr*> jump_from;
    
    void C() { intval=intval2=0; strval=""; p1=NULL; p2=NULL; D(); }
    void D() { bits=0; has_address=false; }

    Expr(ExprType t): type(t) { D(); }
    Expr(ExprType t, unsigned i): type(t), intval(i) { D(); }
    Expr(ExprType t, unsigned i, unsigned i2): type(t), intval(i), intval2(i2) { D(); }
    Expr(ExprType t, int i): type(t), intval(i) { D(); }
    Expr(ExprType t, bool b): type(t), boolval(b) { D(); }
    //Expr(ExprType t, RegisterNameType v): type(t), intval(v) { D(); }
    Expr(ExprType t, const std::string& s): type(t), strval(s) { D(); }
    Expr(ExprType t, const char* s): type(t), strval(s) { D(); }
    Expr(ExprType t, Expr* a, Expr* b=NULL): type(t), p1(a), p2(b) { D(); }
    Expr(ExprType t, Expr* a, unsigned i): type(t), intval(i), p1(a) { D(); }
    Expr(ExprType t, Expr* a, unsigned i, unsigned i2): type(t), intval(i), intval2(i2), p1(a) { D(); }
    void Redefine(ExprType t) { C(); type=t; }
    void Redefine(ExprType t, unsigned i) { C(); type=t; intval=i; }
    void Redefine(ExprType t, unsigned i, unsigned i2) { C(); type=t; intval=i; intval2=i2; }
    void Redefine(ExprType t, int i) { C(); type=t; intval=i; }
    void Redefine(ExprType t, bool b) { C(); type=t; boolval=b; }
    //void Redefine(ExprType t, RegisterNameType v) { C(); type=t; intval=v; }
    void Redefine(ExprType t, const std::string& s) { C(); type=t; strval=s; }
    void Redefine(ExprType t, const char* s) { C(); type=t; strval=s; }
    void Redefine(ExprType t, Expr* a, Expr* b=NULL) { C(); type=t; p1=a; p2=b; }
    void Redefine(ExprType t, Expr* a, unsigned i) { C(); type=t; p1=a; intval=i; }
    void Redefine(ExprType t, Expr* a, unsigned i, unsigned i2) { C(); type=t; p1=a; intval=i; intval2=i2; }
    
    void TraceUsage(ReferenceRecord& rec, ExprType t, unsigned param)
    {
        if(type == Call || type == Asm)
        {
            // Assume everything is done because we don't know
            rec.AssumeWorst();
            return;
        }
    
        if(type == t)
        	if((t == Register && intval == param)
        	|| (p1 && p1->IsConst() && p1->GetConst() == param))
                rec.AddUsed();
        
        if(p2) p2->TraceUsage(rec, t, param);
        if(type == Assign)
        {
            if(p1)
            {
                if(p1->type == t)
                    if((t == Register && p1->intval == param)
                    || (p1->p1 && p1->p1->IsConst() && p1->p1->GetConst() == param))
                        rec.SetRedefined();
                if(p1->p1) p1->p1->TraceUsage(rec, t, param);
                if(p1->p2) p1->p2->TraceUsage(rec, t, param);
            }
            return;
        }
        if(p1) p1->TraceUsage(rec, t, param);
    }
    
    void Redefine(autoptr<Expr>& where,
                  ExprType t,
                  const std::map<unsigned, autoptr<Expr> >& values,
                  bool self=true)
    {
        bool next_self = type != Assign;
        if(p1) p1->Redefine(p1, t, values, next_self);
        if(p2) p2->Redefine(p2, t, values);
        
        if(self && type == t)
        {
            unsigned val;
            if(t == Register) val = intval;
            else { if(!IsConst()) return; val = GetConst(); }
            std::map<unsigned, autoptr<Expr> >::const_iterator i;
            i = values.find(val);
            if(i != values.end())
            {
                where = i->second;
            }
        }
    }
    
    void SetBitness(int b) { bits=b; }
    void SetAddress(unsigned a) { address=a; has_address=true; }
    
    static void ShowIndent() { std::printf("%*s", IndentLevel, ""); }
public:
    unsigned         intval, intval2;
    std::string      strval;
    bool             boolval;
    autoptr<Expr> p1, p2;
    
    void Dump() const
    {
        DumpCommon();
        switch(type)
        {
            case Constant: std::printf("$%X", intval); break;
            case NamedConstant: std::printf("%s", strval.c_str()); break;
            case Register: std::printf("%c", intval); break;
            case DirectAddr:
            case DataAddr:
            case AbsAddr: std::printf("["); p1->Dump(); std::printf("]"); break;
            case StackDirect: std::printf("["); p1->Dump(); std::printf("+S]"); break;
            case Add: p1->Dump(); std::printf("+"); p2->Dump(); break;
            case Sub: p1->Dump(); std::printf("-"); p2->Dump(); break;
            case Adc: p1->Dump(); std::printf(" adc "); p2->Dump(); break;
            case Sbc: p1->Dump(); std::printf(" sbc "); p2->Dump(); break;
            case Or:  p1->Dump(); std::printf(" | "); p2->Dump(); break;
            case And: p1->Dump(); std::printf(" & "); p2->Dump(); break;
            case AndNot: p1->Dump(); std::printf(" & ~ "); p2->Dump(); break;
            case Xor: p1->Dump(); std::printf(" xor "); p2->Dump(); break;
            case Div: p1->Dump(); std::printf(" /"); p2->Dump(); break;
            case Mod: p1->Dump(); std::printf(" mod "); p2->Dump(); break;
            case Mul: p1->Dump(); std::printf(" * "); p2->Dump(); break;
            case Cmp: std::printf("cmp "); p1->Dump(); std::printf(", "); p2->Dump(); break;
            case Bit: std::printf("bit "); p1->Dump(); std::printf(", "); p2->Dump(); break;
            case Assign: p1->Dump(); std::printf(" = "); p2->Dump(); break;
            case Push: std::printf("push "); p1->Dump(); break;
            case Pop: std::printf("pop "); p1->Dump(); break;
            case RtsSec: std::printf("sec:rts"); break;
            case RtsClc: std::printf("clc:rts"); break;
            case Rts: std::printf("rts"); break;
            case Asm: std::printf(";%s", strval.c_str()); break;
            case Comment: std::printf("; %s", strval.c_str()); break;
            case Call: std::printf("call %s", strval.c_str()); break;
            case Goto: std::printf("goto $%06X", intval); break;
            case CarryTest: std::printf("%s%s", boolval?"":"not ", "carry"); break;
            case ZeroTest: std::printf("%s%s", boolval?"":"not ", "zero"); break;
            case SignTest: std::printf("%s%s", boolval?"":"not ", "sign"); break;
            case OverflowTest: std::printf("%s%s", boolval?"":"not ", "overflow"); break;
            case IfGoto: std::printf("if "); p1->Dump();
                         std::printf(" then goto $%06X else goto $%06X",
                                     intval, intval2); break;
        }
    }
    
    bool IsTerminal() const
    {
        return type == RtsClc
            || type == RtsSec
            || type == Rts
            || type == Goto
            || type == IfGoto;
    }
    
    bool IsConst() const { return type == Constant; }
    unsigned GetConst() const { return intval; }

protected:
    static int IndentLevel;
    static void Indent(int diff) { IndentLevel += diff; }

    void DumpCommon() const
    {
        bool needlf = false;
        if(has_address) { needlf = true; std::printf("\r@%06X\n", address); }
        for(std::set<std::string>::const_iterator
            i = labels.begin(); i != labels.end(); ++i)
        {
            needlf = true;
            std::printf("\r%s:\n", i->c_str());
        }
        if(needlf) { ShowIndent(); needlf = false; }
        
        if(!jump_from.empty())
        {
            for(std::list<const Expr*>::const_iterator
                i = jump_from.begin(); i != jump_from.end(); ++i)
            {
                std::printf("; from $%06X\n", (*i)->address);
                ShowIndent();
            }
        }
        
        if(bits)std::printf("<%d>", bits);
    }
};

typedef autoptr<Expr> exprp;

#endif

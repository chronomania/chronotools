#include <iostream>
#include <cstdio>
#include <set>
#include <map>
#include <list>
#include <vector>

#include "wstring.hh"
#include "config.hh"
#include "ctcset.hh"

#include "casegen.hh"
#include "codegen.hh"

#include "autoptr"

#define DEBUG_TABLECODE 0

#define CONVERT_RULETREES 1

#undef EIRATEST

namespace
{
    /* TODO: Tail optimization */

    /* Emit any code taking an int param */
    const std::string CreateImmedIns(const std::string& s, int value)
    {
        char Buf[64]; std::sprintf(Buf, " #$%X", value);
        return s + Buf;
    }

    const std::string CreateStackIns(const std::string& s, unsigned index)
    {
        char Buf[64]; std::sprintf(Buf, " $%u,s", index);
        return s + Buf;
    }
    
    bool A_is_16bit()
    {
        const FlagAssumption flags = GetAssumption();
        return flags.GetA() == Assume16bitA;
    }
    bool A_is_8bit()
    {
        const FlagAssumption flags = GetAssumption();
        return flags.GetA() == Assume8bitA;
    }
    bool XY_is_16bit()
    {
        const FlagAssumption flags = GetAssumption();
        return flags.GetXY() == Assume16bitXY;
    }
    
    enum CarryAssumedState { CarrySet, CarryUnset, CarryUnknown };
    class CarryAssumption
    {
        CarryAssumedState state;
        bool specified;
    public:
        CarryAssumption()
           : state(CarryUnknown), specified(false) { }
        CarryAssumption(CarryAssumedState C)
           : state(C), specified(true) { }
        void Combine(const CarryAssumption& b)
        {
            //Dump("Combining: ");
            //b.Dump(" and: ");
            
            if(!b.specified) return;
            if(!specified) { state = b.state; }
            else if(state != b.state) { state=CarryUnknown; }
            specified=true;
            //Dump("Result: ");
        }
        void Assume(CarryAssumedState s) { state=s; specified=true; }
        void Undefine() { specified = false; state = CarryUnknown; }
        void Dump(const std::string& prefix="") const
        {
            if(!specified)
                Emit(std::string("nop;")+prefix+"carry insignificant");
            else if(state == CarrySet)
                Emit(std::string("nop;")+prefix+"carry set");
            else if(state == CarryUnset)
                Emit(std::string("nop;")+prefix+"carry unset");
            else
                Emit(std::string("nop;")+prefix+"carry unknown");
        }
        bool operator== (CarryAssumedState s) const { return state == s; }
        bool operator!= (CarryAssumedState s) const { return state != s; }
    };
    
    CarryAssumption assumed_carry;
    const CarryAssumption& GetCarryAssumption() { return assumed_carry; }
    void Assume(const CarryAssumption &s) { assumed_carry = s; }
    void Assume(CarryAssumedState s) { assumed_carry.Assume(s); }
    void UndefineCarry() { assumed_carry.Undefine(); }
    void DumpCarry(const std::string& prefix="") { assumed_carry.Dump(prefix); }

    class Assembler
    {
        std::wstring CurSubName;
        
        struct variable
        {
            unsigned stackpos;
            bool read,written,loaded;
            bool is_regvar;
            
            variable() : stackpos(0),read(false),written(false),loaded(false),
                         is_regvar(false)
            {
            }
        };
        
        typedef map<std::wstring, variable> vars_t;
        vars_t vars;

        bool started;       // Code begun?
        unsigned LoopCount; // How many loops active
        
        struct BranchStateData
        {
            FlagAssumption  flags;
            CarryAssumption carry;
            
            void CombineCurrent()
            {
                flags.Combine(GetAssumption());
                carry.Combine(GetCarryAssumption());
            }
        public:
            BranchStateData(): flags(), carry() { }
        };
        
        struct BranchData
        {
            unsigned level;
            std::string label;
            
            BranchStateData state;
        public:
            BranchData(): level(), label(), state() { }
        };
        typedef std::list<BranchData> branchlist_t;
        branchlist_t openbranches;
        
        struct FunctionData
        {
            FlagAssumption flags;
            unsigned    varcount;
        public:
            FunctionData(): flags(), varcount(0) { }
        };
        typedef map<std::wstring, FunctionData> functionlist_t;
        functionlist_t functiondata;
        
        std::wstring PendingCall;
        
        class ConstState
        {
            bool known;
            unsigned value;
        public:
            ConstState() : known(false),value(0) {}
        
            void Invalidate() { known=false; }
            bool Known() const { return known; }
            void Set(unsigned v) { known=true; value=v; }
            bool Is(unsigned v) const { return known && value==v; }
            void Inc() { if(known) ++value; }
            void Dec() { if(known) --value; }
        };

        class VarState
        {
            bool known;
            std::wstring var;
        public:
            VarState() : known(false) {}
            void Invalidate() { known=false; }
            bool Known() const { return known; }
            void Set(const std::wstring &v) { known=true; var=v; }
            bool Is(const std::wstring &v) const { return known && var==v; }
        };
        
        struct regstate
        {
            ConstState Const;
            VarState Var;
        public:
            regstate(): Const(), Var() { }
                
            void Invalidate() { Const.Invalidate(); Var.Invalidate(); }
        };
        
        regstate ALstate, AHstate;
        
        void Invalidate_A()
        {
            ALstate.Invalidate();
            AHstate.Invalidate();
        }
        
        unsigned CountMemVariables() const
        {
            // Count variables
            unsigned varcount = vars.size();
            // But don't count register variables
            for(vars_t::const_iterator i=vars.begin(); i!=vars.end(); ++i)
                if(i->second.is_regvar)
                    --varcount;
            
            return varcount;
        }
        
        void DeallocateVars()
        {
            unsigned varcount = CountMemVariables();
            
            if(varcount >= 6
            || varcount == 4
            || XY_is_16bit())
            {
                while(varcount >= 2)
                {
                    Emit("ply", Want16bitXY);
                    varcount -= 2;
                }
            }

            while(varcount >= 1)
            {
                Emit("ply", Want8bitXY);
                varcount -= 1;
            }
        }
        
        void FRAME_BEGIN()
        {
            unsigned varcount = CountMemVariables();
            functiondata[CurSubName].varcount = varcount;
            
            /* Note: All vars will be initialized with A's current value */
        #if 0
            // That's why this is disabled
            while(varcount >= 2)
            {
                Emit("pha", Want16bitA);
                varcount -= 2;
            }
        #endif
            while(varcount >= 1)
            {
                Emit("pha", Want8bitA);
                varcount -= 1;
            }

            Invalidate_A();
            ALstate.Var.Set(MagicVarName);
            ALstate.Const.Invalidate();
        }
        void FRAME_END()
        {
            if(CurSubName.empty()) return;
            
            for(vars_t::const_iterator
                i = vars.begin();
                i != vars.end();
                ++i)
            {
                const char *warning = 0;
                if(!i->second.read)
                {
                    if(i->second.written) warning = "was written but never read";
                    else warning = "was never written or read";
                }
                else if(!i->second.loaded && !i->second.is_regvar)
                {
                    warning = "should be defined as REG";
                }
                if(warning)
                {
                    fprintf(stderr, "  Warning: In function '%s', variable '%s' %s.\n",
                        WstrToAsc(CurSubName).c_str(),
                        WstrToAsc(i->first).c_str(),
                        warning);
                }
            }
            
            FINISH_BRANCHES();
            //VOID_RETURN(); - assume we've already done it
        }
        void FINISH_BRANCHES()
        {
            openbranches.clear();
        }
        void CheckPendingCall()
        {
            if(PendingCall.empty()) return;
            
            // leave A unmodified and issue call to function

            // X needs to be saved if we're in a loop.
            if(LoopCount)
            {
                Emit("phx", Want16bitXY);
            }
            
            EmitBranch("jsr", WstrToAsc(PendingCall));
            Assume(CarryUnknown);

            // No idea what A contains after function call.
            Invalidate_A();
            
            if(PendingCall == OutcHelperName)
            {
                // OutcHelper is assumed to return
                // with X=16bit, A=16bit
                Assume(Assume16bitXY, Assume16bitA);
            }
            else
            {
                const FlagAssumption& flags = functiondata[PendingCall].flags;
                Assume(flags.GetA(), flags.GetXY());
            }
            
            if(LoopCount)
            {
                Emit("plx", Want16bitXY);
            }
            
            PendingCall.clear();
        }
        void CheckCodeStart()
        {
            CheckPendingCall();
            if(started) return;
            
            FRAME_BEGIN();
            started = true;
        }
        unsigned char GetStackOffset(const std::wstring &varname) const
        {
            vars_t::const_iterator i = vars.find(varname);
            if(i == vars.end())
            {
                fprintf(stderr, "ERROR: In function '%s': "
                                "Undefined variable '%s'\n",
                    WstrToAsc(CurSubName).c_str(), WstrToAsc(varname).c_str());
                return 0;
            }
            if(i->second.is_regvar)
            {
                fprintf(stderr, "ERROR: In function '%s': "
                                "'%s defined with REG, but isn't in registers\n",
                     WstrToAsc(CurSubName).c_str(), WstrToAsc(varname).c_str());
                return 0;
            }
            
            return i->second.stackpos;
        }
        
        void RememberBranch(const std::string& name, unsigned ind,
                            const FlagAssumption& flags,
                            const CarryAssumption& carry)
        {
            // Remember that when entering this level,
            // must cancel this label.
            
            BranchData tmp;
            tmp.level = ind;
            tmp.label = name;
            tmp.state.flags = flags;
            tmp.state.carry = carry;
            
            openbranches.push_back(tmp);
        }
        void RememberBranch(const std::string& name, unsigned ind)
        {
            FlagAssumption flags = GetAssumption();
            CarryAssumption carry = GetCarryAssumption();
            RememberBranch(name, ind, flags, carry);
        }
        void RememberBranch(const std::string& name, unsigned ind,
                            const FlagAssumption& flags)
        {
            CarryAssumption carry = GetCarryAssumption();
            RememberBranch(name, ind, flags, carry);
        }
        void RememberBranch(const std::string& name, unsigned ind,
                            const CarryAssumption& carry)
        {
            FlagAssumption flags = GetAssumption();
            RememberBranch(name, ind, flags, carry);
        }
        
        void EndBranch(BranchData& data)
        {
            CheckCodeStart();
            
            //data.state.carry.Dump("Saved: ");
            //DumpCarry("Current: ");
            
            data.state.CombineCurrent();
            
            Invalidate_A();

            EmitLabel(data.label);
            Assume(data.state.flags.GetA(), data.state.flags.GetXY());
            Assume(data.state.carry);
            
            //DumpCarry("Result: ");
        }
        
        void GenerateComparison
            (unsigned indent,
             const std::string& notjump,
             const std::string& jump)
        {
            std::string elselabel = GenLabel();
            
            if(notjump == "bcc")
            {
                if(GetCarryAssumption() == CarrySet) return;
                if(GetCarryAssumption() == CarryUnset)
                {
                    EmitBranch("bra", elselabel);
                    EmitBarrier();
                    UndefineCarry();
                    return;
                }
                //Emit("nop;bcc");
                EmitBranch("bcc", elselabel);
                RememberBranch(elselabel, indent, CarryUnset);
                Assume(CarrySet);
                return;
            }
            if(notjump == "bcs")
            {
                if(GetCarryAssumption() == CarryUnset) return;
                if(GetCarryAssumption() == CarrySet)
                {
                    EmitBranch("bra", elselabel);
                    EmitBarrier();
                    UndefineCarry();
                    return;
                }
                //Emit("nop;bcs");
                EmitBranch("bcs", elselabel);
                RememberBranch(elselabel, indent, CarrySet);
                Assume(CarryUnset);
                return;
            }
            
            EmitBranch(notjump, elselabel);
            RememberBranch(elselabel, indent);
        }

    public:
        const std::wstring LoopHelperName;
        const std::wstring OutcHelperName;
        const std::wstring MagicVarName;
        
        Assembler()
        : CurSubName(),
          vars(),
          started(),
          LoopCount(),
          openbranches(),
          functiondata(),
          PendingCall(),
          ALstate(), AHstate(),
          LoopHelperName(GetConf("compiler", "loophelpername")),
          OutcHelperName(GetConf("compiler", "outchelpername")),
          MagicVarName(GetConf("compiler", "magicvarname")),
          LoopStack()
        {
            Assume(UnknownXY, UnknownA);
            Assume(CarryUnknown);
            EmitSegment("code");
            LoopCount=0;
        }
        
        void INDENT_LEVEL(unsigned level)
        {
            // Go the label list in REVERSE order; First-in, last-out
            for(branchlist_t::reverse_iterator
                a = openbranches.rbegin();
                a != openbranches.rend();
                ++a)
            {
                if(a->level != 999 && level <= a->level)
                {
                    EndBranch(*a);
                    a->level = 999; // prevent being reassigned
                }
            }
        }
        void START_FUNCTION(const std::wstring &name)
        {
            CurSubName = name;
            started = false;
            vars.clear();
            
            EmitIfNDef(std::string("OMIT_") + WstrToAsc(name));
            
            EmitLabel(WstrToAsc(name));
            KeepLabel(WstrToAsc(name));
            Assume(UnknownXY, UnknownA);
            Assume(CarryUnknown);
            
            Invalidate_A();
        }
        void END_FUNCTION()
        {
            CheckCodeStart();
            FRAME_END();
            if(!CurSubName.empty())
            {
                EmitEndIfDef();
            }
        }
        void DECLARE_VAR(const std::wstring &name)
        {
            if(vars.find(name) == vars.end())
            {
                // stack addresses get smaller when pushing.
                //    s = 6E5
                //   pha - this goes to 6E5
                //   pha - this goes to 6E4
                //   pha - this goes to 6E3
                // after that, s = 6E2.
                // Thus, [s+1] refers to var1, [s+2] to var2, and so on.
                
                unsigned stackpos = vars.size() + 1;
                
                // FIXME: Should order the vars so that reg vars are last!
                // In order to get proper stack positions.
                
                vars[name].is_regvar = false;
                vars[name].stackpos = stackpos;
            }
        }
        void DECLARE_REGVAR(const std::wstring &name)
        {
            vars[name].is_regvar = true;
        }
        void VOID_RETURN()
        {
            if(!PendingCall.empty())
            {
                std::string Target = WstrToAsc(PendingCall);
                PendingCall.clear();
                CheckCodeStart();
                DeallocateVars();
                
                EmitBranch("brl", Target);
                EmitBarrier();
                UndefineCarry();

                return;
            }
            CheckCodeStart();
            
            DeallocateVars();
            
            functiondata[CurSubName].flags.Combine(GetAssumption());
            Emit("rts");
            EmitBarrier();
            UndefineCarry();
        }
        void BOOLEAN_RETURN(bool value)
        {
            CheckCodeStart();
            // emit flag, then return
            
            unsigned varcount = CountMemVariables();

            if(value)
            {
                if(GetCarryAssumption() != CarrySet)
                {
                    // sec = true
                    if(varcount > 0 && (varcount <= 3 || varcount == 5))
                    {
                        // Handy to set XY=8bit here too
                        Emit("sec", Want8bitXY);
                    }
                    else
                    {
                        Emit("sec");
                    }
                    Assume(CarrySet);
                }
                else
                {
                    Emit(";carry already set");
                }
                VOID_RETURN();
            }
            else
            {
                if(GetCarryAssumption() != CarryUnset)
                {
                    // clc = false
                    
                    if(varcount > 0 && !(varcount & ~ 2))
                    {
                        // Handy to set XY=16bit here too
                        Emit("clc", Want16bitXY);
                    }
                    else
                    {
                        Emit("clc");
                    }
                    Assume(CarryUnset);
                }
                else
                {
                    Emit(";carry already unset");
                }
                VOID_RETURN();
            }
        }
        
        bool IsCached(const std::wstring &name) const
        {
            return ALstate.Var.Is(name);
        }
        
        void LOAD_VAR(const std::wstring &name, bool need_16bit = false)
        {
            CheckCodeStart();
            // load var to A
            
            if(vars.find(name) != vars.end())
            {
                vars[name].read = true;

                if(ALstate.Var.Is(name))
                {
                    /* AL already has the value. */
                    if(need_16bit && !AHstate.Const.Is(0))
                    {
                        Emit(CreateImmedIns("and", 0xFF), Want16bitA);
                        AHstate.Const.Set(0);
                        AHstate.Var.Invalidate();
                    }
                    return;
                }

                unsigned stackpos = GetStackOffset(name);
                vars[name].loaded = true;
                
                if(!vars[name].written)
                {
                    // Now only gives this warning for memory vars.
                    
                    fprintf(stderr,
                        "  Warning: In function '%s', variable '%s' was read before written.\n",
                        WstrToAsc(CurSubName).c_str(), WstrToAsc(name).c_str());
                }
                
                if(need_16bit && !AHstate.Const.Is(0))
                {
                    if(ALstate.Const.Is(0))
                    {
                        // AL is 0. We get easily 0 to AH.
                        Emit("xba");
                        AHstate = ALstate;
                        // AL is now what AH was previously,
                        // but since we're going to change AL
                        // anyway, not worth stating the state.
                    }
                    else
                    {
                        if(A_is_16bit())
                        {
                            /* causes too much bitness changes */
                            Emit(CreateStackIns("lda", stackpos), Want16bitA);
                            Emit(CreateImmedIns("and", 0xFF),     Want16bitA);
                            AHstate.Const.Set(0);
                            AHstate.Var.Invalidate();
                            ALstate.Var.Set(name);
                            ALstate.Const.Invalidate();
                            return;
                            /*
                             Emit("lda #0");
                             ALstate.Const.Set(0);
                             ALstate.Var.Invalidate();
                            */
                        }
                        else
                        {
                            Emit("lda #0",  Want8bitA);
                            Emit("xba");
                            ALstate = AHstate; // AL has now what was in AH
                            AHstate.Const.Set(0); // And AH is 0 as we wanted
                            AHstate.Var.Invalidate();
                        }
                    }
                }
                
                Emit(CreateStackIns("lda", stackpos), Want8bitA);

                ALstate.Var.Set(name);
                ALstate.Const.Invalidate();
                
                return;
            }
            
            unsigned val = 0;
            
            if(name[0] == '\'')
                val = getctchar(name[1], cset_12pix);
            else
                val = strtol(WstrToAsc(name).c_str(), NULL, 10);
            
            if(val >= 256)
            {
                fprintf(stderr,
                    "  Warning: In function '%s', numeric constant %u too large (>255)\n",
                    WstrToAsc(CurSubName).c_str(), val);
            }
            
            unsigned lo = val&255;
            unsigned hi = val>>8;
            
            if(A_is_16bit()
            || (need_16bit && !AHstate.Const.Is(hi)))
            {
                Emit(CreateImmedIns("lda", val), Want16bitA);
                ALstate.Const.Set(lo);
                ALstate.Var.Invalidate();
                AHstate.Const.Set(hi);
                AHstate.Var.Invalidate();
                return;
            }
            if(!ALstate.Const.Is(lo))
            {
                Emit(CreateImmedIns("lda", lo), Want8bitA);
                ALstate.Const.Set(lo);
                ALstate.Var.Invalidate();
            }
        }
        void STORE_VAR(const std::wstring &name)
        {
            CheckCodeStart();
            
            if(ALstate.Var.Is(name)) return;

            if(vars.find(name) == vars.end()
            || !vars.find(name)->second.is_regvar)
            {
                // store A to var
                unsigned stackpos = GetStackOffset(name);
                Emit(CreateStackIns("sta", stackpos), Want8bitA);
            }
            vars[name].written = true;
            
            ALstate.Var.Set(name);
        }
        void INC_VAR(const std::wstring &name)
        {
            CheckCodeStart();
            // inc var
            LOAD_VAR(name);
            Emit("inc"); // A bitness doesn't really matter here

            ALstate.Var.Invalidate();
            ALstate.Const.Inc();

            STORE_VAR(name);
            
            // curiously, lda&inc&sta don't touch carry.
        }
        void DEC_VAR(const std::wstring &name)
        {
            CheckCodeStart();
            // dec var
            LOAD_VAR(name);
            Emit("dec"); // A bitness doesn't really matter here

            ALstate.Var.Invalidate();
            ALstate.Const.Dec();

            STORE_VAR(name);
        }
        void CALL_FUNC(const std::wstring &name)
        {
            CheckCodeStart();
            
            PendingCall = name;
        }
        void COMPARE_BOOL(unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if boolean set
            
            GenerateComparison(indent, "bcc", "bcs");
        }
        void COMPARE_EQUAL(const std::wstring &name, unsigned indent)
        {
            CheckCodeStart();
            
            if(vars.find(name) != vars.end())
            {
                unsigned stackpos = GetStackOffset(name);
                vars[name].read = vars[name].loaded = true;
                
                // process subblock if A is equal to given var
                // Note: Checking ALstate would be useless here.
                Emit(CreateStackIns("cmp", stackpos), Want8bitA);
            }
            else
            {
                unsigned val = 0;
                
                if(name[0] == '\'')
                    val = getctchar(name[1], cset_12pix);
                else
                    val = strtol(WstrToAsc(name).c_str(), NULL, 10);
                
                if(!A_is_16bit() && val >= 256)
                {
                    fprintf(stderr,
                        "  Warning: In function '%s', numeric constant %u too large (>255)\n",
                        WstrToAsc(CurSubName).c_str(), val);
                }
                // A bitness insignificant
                Emit(CreateImmedIns("cmp", val));
            }
            Assume(CarryUnknown);
            
            GenerateComparison(indent, "bne", "beq");
        }
        void COMPARE_ZERO(const std::wstring &name, unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if var is zero
            LOAD_VAR(name);

            // Note: we're not checking ALstate here, would be mostly useless check
            
            GenerateComparison(indent, "bne", "beq");
        }
        void SELECT_CASE(const std::wstring &cset, unsigned indent)
        {
            class CaseHandler: public CaseGenerator
            {
                bool      LastWasBra;
                bool      ChainCompare;
                CaseValue LastCompare;
                bool      BraPending;
                std::string    PendingLabel;
                std::string    LastCompareType;
                
                std::map<std::string, BranchStateData> jumps;
                
            private:
                void EmitCompareJump
                   (CaseValue value,
                    const std::string& target,
                    const std::string& comparetype)
                {
                    CheckPendingBra();
                    if(!ChainCompare || value != LastCompare)
                    {
                        Emit(CreateImmedIns("cmp", value), Want8bitA);
                        Assume(CarryUnknown);
                        LastCompare  = value;
                        ChainCompare = true;
                    }
                    
                    if(comparetype == "bcc")
                    {
                        jumps[target].flags.Combine(GetAssumption());
                        jumps[target].carry.Combine(CarryUnset);
                        EmitBranch(comparetype, target);
                        Assume(CarrySet);
                    }
                    else if(comparetype == "bcs")
                    {
                        jumps[target].flags.Combine(GetAssumption());
                        jumps[target].carry.Combine(CarrySet);
                        EmitBranch(comparetype, target);
                        Assume(CarryUnset);
                    }
                    else
                    {
                        jumps[target].CombineCurrent();
                        EmitBranch(comparetype, target);
                    }
                    
                    LastWasBra      = false;
                    LastCompareType = comparetype;
                }
                void CheckPendingBra()
                {
                    if(!BraPending) return;
                    jumps[PendingLabel].CombineCurrent();
                    EmitBranch("bra", PendingLabel);
                    EmitBarrier();
                    UndefineCarry();
                    BraPending = false;
                }

            public:
                CaseHandler(class Assembler& a,
                            const std::string &posilabel)
                : LastWasBra(false), ChainCompare(false),
                  LastCompare(),
                  BraPending(false),
                  PendingLabel(posilabel),
                  LastCompareType(),
                  jumps()
                {
                }
                
                virtual CaseValue GetMinValue()    const { return 0;   }
                virtual CaseValue GetMaxValue()    const { return 255; }
                virtual CaseValue GetDefaultCase() const { return -1;  }

                virtual void EmitJump(const std::string& target)
                {
                    if(LastWasBra) return;
                    if(target == PendingLabel)
                        BraPending = true;
                    else
                    {
                        jumps[target].CombineCurrent();
                        EmitBranch("bra", target);
                        EmitBarrier();
                        UndefineCarry();
                    }
                    LastWasBra   = true;
                    ChainCompare = false;
                }
                virtual void EmitCompareEQ(CaseValue value, const std::string& target)
                {
                    EmitCompareJump(value, target, "beq");
                }
                virtual void EmitCompareNE(CaseValue value, const std::string& target)
                {
                    EmitCompareJump(value, target, "bne");
                }
                virtual void EmitCompareLT(CaseValue value, const std::string& target)
                {
                    if(value == GetMinValue())
                        { /* nothing */ }
                    else if(value == GetMinValue()+1
                    && (!ChainCompare || value != LastCompare))
                        EmitCompareEQ(value-1, target);
                    else
                        EmitCompareJump(value, target, "bcc");
                }
                virtual void EmitCompareGE(CaseValue value, const std::string& target)
                {
                    if(value == GetMinValue())
                        { EmitJump(target); return; }
                    else if(value == GetMaxValue())
                        EmitCompareEQ(value, target);
                    else
                        EmitCompareJump(value, target, "bcs");
                }
                virtual void EmitCompareLE(CaseValue value, const std::string& target)
                {
                    if(value == GetMaxValue())
                        { EmitJump(target); return; }
                    else if(value == GetMinValue())
                        EmitCompareEQ(value, target);
                    else if(ChainCompare && value == LastCompare && LastCompareType == "beq")
                        EmitCompareLT(value, target);
                    else
                        EmitCompareLT(value+1, target);
                }
                virtual void EmitCompareGT(CaseValue value, const std::string& target)
                {
                    if(value == GetMaxValue())
                        { /* nothing */ }
                    else if(ChainCompare && value == LastCompare && LastCompareType == "beq")
                        EmitCompareGE(value, target);
                    else
                        EmitCompareGE(value+1, target);
                }
                virtual void EmitSubtract(CaseValue value)
                {
                    CheckPendingBra();
                    
                    /*
                    if(A_is_8bit() && GetCarryAssumption() == CarryUnset)
                    {
                        if(GetCarryAssumption() != CarryUnset)
                        {
                            Emit("clc", Want8bitA);
                            Assume(CarryUnset);
                        }
                        Emit(CreateImmedIns("adc", 256-value), Want8bitA);
                    }
                    else*/
                    {
                        if(GetCarryAssumption() != CarrySet)
                        {
                            Emit("sec", Want8bitA);
                            Assume(CarrySet);
                        }
                        Emit(CreateImmedIns("sbc", value), Want8bitA);
                    }
                    Assume(CarryUnknown);
                    LastWasBra   = false;
                }
                virtual const std::string EmitBlock()
                {
                    return GenLabel();
                }
                virtual void EmitEndBlock(const std::string& label)
                {
                    CheckPendingBra();
                    EmitLabel(label);
                    
                    FlagAssumption  flags = GetFlags(label);
                    CarryAssumption carry = GetCarry(label);
                    Assume(flags.GetA(), flags.GetXY());
                    Assume(carry);
                    
                    LastWasBra   = false;
                }
                virtual void EmitJumpTable(const std::vector<std::string>& table)
                {
                    std::string tablelabel = GenLabel();
                    CheckPendingBra();
                    //if(!Asm.AHstate.Const.Is(0))
                    {
                        // We don't have Asm handle here
                        Emit(CreateImmedIns("and",255), Want16bitA, Want16bitXY);
                    }
                    Emit("asl", Want16bitA, Want16bitXY);
                    Assume(CarryUnset); // assume the shift didn't overflow.
                    Emit("tax", Want16bitA, Want16bitXY);
                    
                    const CarryAssumption carry = GetCarryAssumption();
                    const FlagAssumption  flags = GetAssumption();
                    
                    //DumpCarry();
                    
                    EmitBranch(".jmpx", tablelabel);
                    EmitBarrier();
                    UndefineCarry();
                    
                    EmitSegment("data");
                    
                    EmitLabel(tablelabel);
                    for(unsigned a=0; a<table.size(); ++a)
                    {
                        const std::string& label = table[a];
                        EmitBranch(".word", label);
                        jumps[label].flags.Combine(flags);
                        jumps[label].carry.Combine(carry);
                        
                        //jumps[label].carry.Dump("^Flags: ");
                    }
                    LastWasBra   = true;
                    
                    EmitBarrier();
                    UndefineCarry();
                    
                    EmitSegment("code");
                }
                
                const FlagAssumption& GetFlags(const std::string& label)
                {
                    return jumps[label].flags;
                }
                
                const CarryAssumption& GetCarry(const std::string& label)
                {
                    return jumps[label].carry;
                }
            };
            
            std::string positivelabel = GenLabel();
            std::string negativelabel = GenLabel();
            
            CaseHandler casehandler(*this, positivelabel);
            CaseItemList cases;
            CaseItem tmpcase;
            for(unsigned a=0; a<cset.size(); ++a)
            {
                // Names can only contain 8bit chars!
                // Note: we're not checking ALstate here, would be mostly useless check
                
                ctchar c = getctchar(cset[a], cset_12pix);
                tmpcase.values.insert(c);
            }
            tmpcase.target = positivelabel;
            cases.push_back(tmpcase);
            casehandler.Generate(cases, negativelabel);
            
            FlagAssumption  flags = casehandler.GetFlags(positivelabel);
            CarryAssumption carry = casehandler.GetCarry(positivelabel);
            EmitLabel(positivelabel);
            Assume(flags.GetA(), flags.GetXY());
            Assume(carry);

            flags = casehandler.GetFlags(negativelabel);
            carry = casehandler.GetCarry(negativelabel);
            RememberBranch(negativelabel, indent, flags, carry);
        }
        
        struct LoopData
        {
            std::string BeginLabel;
            std::string EndLabel;
            
            FlagAssumption EndFlags;
        };
        
        std::list<LoopData> LoopStack;
        void START_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            LoopData data;
            data.BeginLabel = GenLabel();
            data.EndLabel   = GenLabel();
            
            ++LoopCount;
            Emit("ldx #0", Want16bitXY);
            
            EmitLabel(data.BeginLabel);
            // A is not really unknown here, but it's difficult
            // to fix - and it's not important, as this jsr doesn't
            // require anything.
            Assume(Assume16bitXY, UnknownA);
            
            EmitBranch("jsr", WstrToAsc(LoopHelperName));
            Assume(Assume16bitXY, Assume8bitA);
            Assume(CarryUnknown);
            
            // LoopHelper is known to give A=8-bit and X=16-bit.
            
            data.EndFlags = GetAssumption();
            EmitBranch("beq", data.EndLabel);

            Invalidate_A();
            AHstate.Const.Set(0);

            STORE_VAR(MagicVarName);// save in "c".
            // mark read, because it indeed has been
            // read (in the loop end condition).
            vars[MagicVarName].read = true;
            
            LoopStack.push_front(data);
        }
        void END_CHARNAME_LOOP()
        {
            LoopData data = *LoopStack.begin();
            LoopStack.pop_front();
            
            CheckCodeStart();
            
            Emit("inx", Want16bitXY);
            EmitBranch("bra", data.BeginLabel);
            EmitBarrier();
            UndefineCarry();

            EmitLabel(data.EndLabel);
            Assume(data.EndFlags.GetA(), data.EndFlags.GetXY());
            Assume(CarryUnknown);
            
            Invalidate_A();
            
            --LoopCount;
        }
        void LOAD_CHARACTER()
        {
            CheckCodeStart();
            Emit("tax", Want16bitXY);
            EmitBranch("jsr", WstrToAsc(LoopHelperName));
            Assume(Assume16bitXY, Assume8bitA);
            Assume(CarryUnknown);
        }
        void LOAD_LAST_CHAR()
        {
            CheckCodeStart();
            Emit("ora #$8000", Want16bitXY, Want16bitA);
            Emit("tax");
            EmitBranch("jsr", WstrToAsc(LoopHelperName));
            Assume(Assume16bitXY, Assume8bitA);
            Assume(CarryUnknown);
        }
        
        void OUT_CHARACTER()
        {
            // outputs character in A
            CALL_FUNC(OutcHelperName);
        }
    };
}

static
const std::vector<std::wstring> Split
  (const std::wstring& text,
   wchar_t separator=L' ',
   wchar_t quote=L'\'',
   bool squish=true)
{
    std::vector<std::wstring> words;
    
    unsigned a=0, b=text.size();
    while(a<b)
    {
        if(text[a] == separator && squish) { ++a; continue; }
        if(text[a] == quote)
        {
            ++a;
            unsigned start = a;
            ++a; // ignore 1 char
            while(a < b && text[a] != quote) ++a;
            words.push_back(text.substr(start-1, a-start+2));
            if(a < b) { ++a; /* skip quote */ }
            continue;
        }
        unsigned start = a;
        while(a<b && text[a] != separator) ++a;
        words.push_back(text.substr(start, a-start));
        if(!squish && a<b && text[a]==separator) ++a;
        continue;
    }
    return words;
}

static
const std::wstring Trim(const std::wstring& s)
{
    unsigned n_begin = 0;
    while(n_begin < s.size() && (s[n_begin] == L' ' || s[n_begin] == L'\t')) ++n_begin;
    unsigned endpos = s.size();
    while(endpos > n_begin && (s[endpos-1] == L' ' || s[endpos-1] == L'\t')) --endpos;
    return s.substr(n_begin, endpos-n_begin);
}
static
const std::wstring GetCSet(const std::set<wchar_t>& chars)
{
    std::wstring result;
    for(std::set<wchar_t>::const_iterator
        i = chars.begin(); i != chars.end(); ++i)
    {
        result += *i;
    }
    return result;
}


static
const std::string ConvStr(const std::wstring& s)
{
    wstringOut conv;
    conv.SetSet(getcharset());
    return conv.puts(s);
}


class TableParser
{
    static const std::wstring GetSetFuncName(wchar_t setch)
    {
        return wformat(L"CheckCharSet_%lc", setch);
    }
    
    struct Component
    {
        enum
        {
            compare_lastch_set,
            compare_lastch_set2,
            compare_lastch_boolfunc,
            compare_boolfunc,
            compare_endfunc
        } type;
        
        unsigned          lastchpos;
        unsigned          lastchpos2;
        std::set<wchar_t> chars;
        std::wstring ref;

    public:
        Component(): type(compare_lastch_set), lastchpos(0), lastchpos2(0) {}
        
        void LastEqSet(unsigned l, const std::set<wchar_t>& s) { lastchpos=l; chars=s; type=compare_lastch_set; }
        void LastEqSet2(unsigned l, unsigned l2)
        {
            lastchpos=std::min(l,l2);
            lastchpos2=std::max(l,l2);
            type=compare_lastch_set2;
        }
        void LastBoolFunc(unsigned l, const std::wstring& s)  { lastchpos=l; ref=s; type=compare_lastch_boolfunc; }
        void SetBoolFunc(const std::wstring& s)   { ref=s; type=compare_boolfunc; }
        void SetEndFunc(const std::wstring& s)   { ref=s; type=compare_endfunc; }
        
        const std::wstring GetCSet() const
        {
            return ::GetCSet(chars);
        }
        
        bool operator==(const Component& b) const
        {
            if(type != b.type) return false;
            if(type==compare_lastch_set || type==compare_lastch_boolfunc) { if(lastchpos!=b.lastchpos) return false; }
            if(type==compare_lastch_set2) { if(lastchpos2!=b.lastchpos2) return false; }
            if(type==compare_boolfunc
            || type==compare_endfunc
            || type==compare_lastch_boolfunc) { if(ref != b.ref) return false; }
            if(type==compare_lastch_set) { if(chars != b.chars) return false; }
            return true;
        }
        bool operator!=(const Component& b) const { return !operator==(b); }
        bool operator< (const Component& b) const
        {
            if(type != b.type) return type < b.type;
            if(type==compare_lastch_set || type==compare_lastch_boolfunc) { if(lastchpos!=b.lastchpos) return lastchpos < b.lastchpos; }
            if(type==compare_lastch_set2) { if(lastchpos2!=b.lastchpos2) return lastchpos2 < b.lastchpos2; }
            if(type==compare_boolfunc
            || type==compare_endfunc
            || type==compare_lastch_boolfunc) { if(ref != b.ref) return ref < b.ref; }
            if(type==compare_lastch_set) { if(chars != b.chars) return GetCSet() < b.GetCSet(); }
            return false;
        }
        
        bool IsCombinableWith(const Component& b) const
        {
            if(type != b.type) return false;
            if(type == compare_boolfunc
            || type == compare_endfunc
            || type == compare_lastch_boolfunc) { if(ref != b.ref) return false; }
            if(type == compare_lastch_set
            || type == compare_lastch_set2
            || type == compare_lastch_boolfunc)
            {
                if(lastchpos != b.lastchpos) return false;
            }
            if(type == compare_lastch_set2)
            {
                if(lastchpos2 != b.lastchpos2) return false;
            }
            return true;
        }
        void CombineWith(const Component& b)
        {
            chars.insert(b.chars.begin(), b.chars.end());
        }
    };
    struct Action
    {
        enum
        {
            output_char,
            output_context,
            output_ref,
            call_function
        } type;
        
        wchar_t ch;
        unsigned last_but;
        std::wstring func;
        unsigned chpos;

    public:
        Action(): type(output_char), ch('?') {}
        
        void OutCh(wchar_t c) { ch=c; type=output_char; }
        void OutCtx(unsigned n) { last_but=n; type=output_context; }
        void OutRef(unsigned c) { chpos=c; type=output_ref; }
        void CallFunc(const std::wstring& s) { func=s; type=call_function; }
        
        bool operator==(const Action& b) const
        {
            if(type != b.type) return false;
            if(type==output_char) { if(ch != b.ch) return false; }
            if(type==output_context) { if(last_but != b.last_but) return false; }
            if(type==output_ref) { if(chpos != b.chpos) return false; }
            if(type==call_function) { if(func != b.func) return false; }
            return true;
        }
        bool operator!=(const Action& b) const { return !operator==(b); }
        bool operator< (const Action& b) const
        {
            if(type != b.type) return type < b.type;
            if(type==output_char) { if(ch != b.ch) return ch < b.ch; }
            if(type==output_context) { if(last_but != b.last_but) return last_but < b.last_but; }
            if(type==output_ref) { if(chpos != b.chpos) return chpos < b.chpos; }
            if(type==call_function) { if(func != b.func) return func < b.func; }
            return false;
        }
    };

    const std::set<wchar_t> BuildCharset(const std::wstring& s) const
    {
        std::set<wchar_t> result;
        for(unsigned a=0; a<s.size(); ++a)
        {
            result.insert(s[a]);
            std::map<wchar_t, wchar_t>::const_iterator i;
            i = l2u.find(s[a]); if(i != l2u.end()) result.insert(i->second);
            i = u2l.find(s[a]); if(i != u2l.end()) result.insert(i->second);
        }
        return result;
    }
    
    struct Rule
    {
        std::set<Component> components;
        
        bool operator==(const Rule& b) const
        {
            return components == b.components;
        }
        bool operator!= (const Rule& b) const { return !operator==(b); }
        bool operator< (const Rule& b) const
        {
            return components < b.components;
        }
        
    public:
        bool HasComponent(const Component& com) const
        {
            return components.find(com) != components.end();
        }
        void DeleteComponent(const Component& com)
        {
            components.erase(com);
        }
        
        bool IsCombinableWith(const Rule& b) const
        {
            if(components.size() != b.components.size()) return false;
            if(components.size() != 1) return false;
            for(std::set<Component>::const_iterator
                i=components.begin(),
                j=b.components.begin(); i!=components.end(); ++i,++j)
            {
                if(!i->IsCombinableWith(*j)) return false;
            }
            return true;
        }
        void CombineWith(const Rule& b)
        {
            std::set<Component> newset;
            
            for(std::set<Component>::iterator
                i=components.begin(),
                j=b.components.begin(); i!=components.end(); ++i,++j)
            {
                Component tmp = *i;
                tmp.CombineWith(*j);
                newset.insert(tmp);
            }
            components = newset;
        }
    
        void GenerateCode(Assembler& Asm, unsigned& indent) const
        {
            for(std::set<Component>::const_iterator
                i=components.begin(); i!=components.end(); ++i)
            {
                Asm.INDENT_LEVEL(indent);

                const Component& com = *i;
                
                switch(com.type)
                {
                    case Component::compare_boolfunc:
                    {
#if DEBUG_TABLECODE
                        std::cout << ConvStr(wformat(L"%*sIF %ls\n", indent,"", com.ref.c_str()));
                        std::cout << std::flush;
#endif
                        Asm.CALL_FUNC(com.ref);
                        Asm.COMPARE_BOOL(indent);
                        indent += 2;
                        break;
                    }
                    case Component::compare_endfunc:
                    {
#if DEBUG_TABLECODE
                        std::cout << ConvStr(wformat(L"%*sRETURN IF %ls\n", indent,"", com.ref.c_str()));
                        std::cout << std::flush;
#endif
                        Asm.CALL_FUNC(com.ref);
                        Asm.COMPARE_BOOL(0);
                        indent += 2;
                        break;
                    }
                    case Component::compare_lastch_boolfunc:
                    {
#if DEBUG_TABLECODE
                        std::cout << ConvStr(wformat(L"%*sIF %ls LastChar[%u]\n", indent,"", com.ref.c_str(), com.lastchpos));
                        std::cout << std::flush;
#endif
#ifdef EIRATEST
                        if(com.lastchpos==0) Asm.LOAD_VAR(L"'a'");
                        else if(com.lastchpos==1) Asm.LOAD_VAR(L"'r'");
                        else if(com.lastchpos==2) Asm.LOAD_VAR(L"'i'");
                        else if(com.lastchpos==3) Asm.LOAD_VAR(L"'E'");
                        else Asm.LOAD_VAR(L"0");
#else
                        Asm.LOAD_VAR(wformat(L"%u", com.lastchpos));
                        Asm.CALL_FUNC(L"LastCharN");
#endif
                        Asm.STORE_VAR(Asm.MagicVarName);
                        Asm.LOAD_VAR(Asm.MagicVarName);
                        Asm.CALL_FUNC(com.ref);
                        Asm.COMPARE_BOOL(indent);
                        indent += 2;
                        break;
                    }
                    case Component::compare_lastch_set:
                    {
#if DEBUG_TABLECODE
                        std::cout << ConvStr(wformat(L"%*s? LastChar[%u] %ls\n",  indent,"", com.lastchpos, com.GetCSet().c_str()));
                        std::cout << std::flush;
#endif
#ifdef EIRATEST
                        if(com.lastchpos==0) Asm.LOAD_VAR(L"'a'");
                        else if(com.lastchpos==1) Asm.LOAD_VAR(L"'r'");
                        else if(com.lastchpos==2) Asm.LOAD_VAR(L"'i'");
                        else if(com.lastchpos==3) Asm.LOAD_VAR(L"'E'");
                        else Asm.LOAD_VAR(L"0");
#else
                        Asm.LOAD_VAR(wformat(L"%u", com.lastchpos));
                        Asm.CALL_FUNC(L"LastCharN");
#endif
                        Asm.STORE_VAR(Asm.MagicVarName);
                        Asm.LOAD_VAR(Asm.MagicVarName);
                        Asm.SELECT_CASE(com.GetCSet(), indent);
                        indent += 2;
                        break;
                    }
                    case Component::compare_lastch_set2:
                    {
#if DEBUG_TABLECODE
#endif
                        //////////// FIXME: not correct yet.
                        fprintf(stderr, "ERROR: compare_lastch_set2 not usable yet\n");
                        Asm.LOAD_VAR(wformat(L"%u", com.lastchpos));
                        Asm.CALL_FUNC(L"LastCharN");
                        Asm.LOAD_VAR(wformat(L"%u", com.lastchpos2));
                        Asm.CALL_FUNC(L"LastCharN");
                        Asm.STORE_VAR(Asm.MagicVarName);
                        Asm.LOAD_VAR(Asm.MagicVarName);
                        Asm.COMPARE_EQUAL(Asm.MagicVarName, indent);
                        indent += 2;
                        break;
                    }
                }
            }
        }
    };
    struct Compilation
    {
        std::vector<Action> actions;
        
        bool operator== (const Compilation& b) const
        {
            return actions == b.actions;
        }
        bool operator!= (const Compilation& b) const { return !operator==(b); }
        bool operator< (const Compilation& b) const
        {
            return actions < b.actions;
        }

    public:
        void GenerateCode(Assembler& Asm, const unsigned indent) const
        {
            Asm.INDENT_LEVEL(indent);
            
            for(unsigned a=0; a<actions.size(); ++a)
            {
                const Action& act = actions[a];
                switch(act.type)
                {
                    case Action::output_char:
                    {
#if DEBUG_TABLECODE
                        std::cout << ConvStr(wformat(L"%*sOUT '%lc'\n", indent,"", act.ch)) << std::flush;
#endif
                        Asm.LOAD_VAR(wformat(L"'%lc'", act.ch), true);
                        Asm.OUT_CHARACTER();
                            break;
                    }
                    case Action::output_context:
                    {
#if DEBUG_TABLECODE
                        std::cout << ConvStr(wformat(L"%*sCALL OutWordBut %u\n", indent,"", act.last_but));
#endif
                        Asm.LOAD_VAR(wformat(L"%u", act.last_but));
                        Asm.CALL_FUNC(L"OutWordBut");
                        break;
                    }
                    case Action::output_ref:
                    {
#if DEBUG_TABLECODE
                        std::cout << ConvStr(wformat(L"%*sOUT LastChar[%u]\n", indent,"",  act.chpos));
#endif
                        Asm.LOAD_VAR(wformat(L"%u", act.chpos));
                        Asm.CALL_FUNC(L"LastCharN");
                        Asm.STORE_VAR(Asm.MagicVarName);
                        Asm.LOAD_VAR(Asm.MagicVarName, true);
                        Asm.OUT_CHARACTER();
                        break;
                    }
                    case Action::call_function:
                    {
#if DEBUG_TABLECODE
                        std::cout << ConvStr(wformat(L"%*sCALL %ls\n", indent,"", act.func.c_str()));
#endif
                        Asm.CALL_FUNC(act.func);
                        break;
                    }
                }
            }
            if(!actions.empty())
            {
#if DEBUG_TABLECODE
                std::cout << ConvStr(wformat(L"%*sTRUE\n", indent,""));
                std::cout << std::flush;
#endif
#if CONVERT_RULETREES
                Asm.BOOLEAN_RETURN(false); // carry clear=found rule
#else
                Asm.VOID_RETURN();
#endif
            }
        }
    };
    
    struct RuleTree;
    typedef autoeqptr<RuleTree> RuleTreePtr;
    struct RuleTree: public ptrable
    {
        std::vector<RuleTreePtr> subtrees;
        Rule rule;
        Compilation compilation;
    
        bool operator== (const RuleTree& b) const
        {
            return rule == b.rule
                && subtrees == b.subtrees
                && compilation == b.compilation;
        }
        bool operator!= (const RuleTree& b) const { return !operator==(b); }
        bool operator< (const RuleTree& b) const
        {
            if(rule != b.rule) return rule < b.rule;
            if(compilation != b.compilation) return compilation < b.compilation;
            return subtrees < b.subtrees;
        }
        
    private:
        void Assimilate(const RuleTree& r)
        {
            rule.components.insert
            (r.rule.components.begin(), 
             r.rule.components.end());
            compilation.actions.insert
            (compilation.actions.end(),
             r.compilation.actions.begin(),
             r.compilation.actions.end());
            subtrees.insert
            (subtrees.end(),
             r.subtrees.begin(),
             r.subtrees.end());
        }
        
        bool HasEqualConsequence(RuleTree& b) const
        {
            return subtrees    == b.subtrees
                && compilation == b.compilation;
        }
        
        bool OptimizeCommonComponents()
        {
            /* Find out how many times each component is used */
            Component best_cond;   //what component?
            unsigned best_count=0; //how many times? (for scoring)
            unsigned best_pos  =0; //where is the first occurance?
            
            /* Note: Must preserve the order, and thus must not have gaps
             * in the matching condition lists.
             */
            
            for(unsigned s=0; s<subtrees.size(); ++s)
            {
                std::map<Component, unsigned> component_amounts;
                
                const Rule& r = subtrees[s]->rule;
                std::set<Component>::const_iterator j;
                
                for(j = r.components.begin(); j != r.components.end(); ++j)
                {
                    unsigned n_found = 1;
                    
                    for(unsigned s2=s+1; s2<subtrees.size(); ++s2)
                    {
                        const Rule& r2 = subtrees[s2]->rule;
                        if(!r2.HasComponent(*j)) break;
                        ++n_found;
                    }
                    if(n_found > best_count)
                    {
                        best_count = n_found;
                        best_pos   = s;
                        best_cond  = *j;
                    }
                }
            }
            
            if(best_count > 1)
            {
                RuleTree* newtree = new RuleTree;
                newtree->rule.components.insert(best_cond);
                
                unsigned n_trees = 0;
                for(unsigned s=best_pos; s<subtrees.size(); ++s, ++n_trees)
                {
                    Rule& r = subtrees[s]->rule;
                    
                    std::set<Component>::iterator j = r.components.find(best_cond);
                    if(j == r.components.end()) break;
                    
                    r.components.erase(j);
                    newtree->subtrees.push_back(subtrees[s]);
                }
                subtrees.erase(subtrees.begin()+best_pos+1,
                               subtrees.begin()+best_pos+n_trees);
                subtrees[best_pos] = newtree;
                return true;
            }
            return false;
        }
        
        void OptimizeCommonResults()
        {
            for(unsigned s=0; s+1<subtrees.size(); ++s)
            {
                RuleTree& t1 =*subtrees[s+0];
                RuleTree& t2 =*subtrees[s+1];
                
                if(t1.rule.IsCombinableWith(t2.rule)
                && t1.HasEqualConsequence(t2))
                {
                    t1.rule.CombineWith(t2.rule);
                    subtrees.erase(subtrees.begin() + (s+1));
                    --s;
                    continue;
                }
            }
        }

    public:
        void Optimize()
        {
        Reoptimize:
            if(compilation.actions.empty() && subtrees.size() == 1)
            {
                RuleTreePtr tmp = *subtrees.begin();
                subtrees.clear();
                Assimilate(*tmp);
                goto Reoptimize;
            }
        
            for(unsigned s=0; s<subtrees.size(); ++s)
                subtrees[s]->Optimize();

            if(OptimizeCommonComponents())
                goto Reoptimize;
            
            OptimizeCommonResults();
        }

        typedef std::map<Compilation, unsigned> ActionUsageMap;
        typedef std::map<RuleTree, std::wstring> RuleTreeConversionMap;
        
        void FindCommonActions(ActionUsageMap& map)
        {
            if(!compilation.actions.empty())
            {
                ++map[compilation];
            }
            for(unsigned s=0; s<subtrees.size(); ++s)
                subtrees[s]->FindCommonActions(map);
        }
        
        void ConvertTreeToFunctions(RuleTreeConversionMap& map)
        {
            for(unsigned s=0; s<subtrees.size(); ++s)
            {
                subtrees[s]->ConvertTreeToFunctions(map);
            }
            
            std::wstring funcname;
            RuleTreeConversionMap::iterator i = map.find(*this);
            if(i == map.end())
            {
                funcname = wformat(L"RuleTree%u", map.size());
                map[*this] = funcname;
            }
            else
                funcname = i->second;
            
            rule.components.clear();
            compilation.actions.clear();
            
            Component tmp;
            tmp.SetEndFunc(funcname);
            rule.components.insert(tmp);
            subtrees.clear();
        }
        
        void ReplaceAllActionByCall(const Compilation& example, const std::wstring& funame)
        {
            if(compilation == example)
            {
                compilation.actions.clear();
                Action act;
                act.CallFunc(funame);
                compilation.actions.push_back(act);
            }
            for(unsigned s=0; s<subtrees.size(); ++s)
                subtrees[s]->ReplaceAllActionByCall(example, funame);
        }

        unsigned CountLastCh() const
        {
            unsigned max_lastch = 0;
            
            for(std::set<Component>::const_iterator
                i=rule.components.begin(); i!=rule.components.end(); ++i)
            {
                switch(i->type)
                {
                    case Component::compare_lastch_set2:
                        max_lastch = std::max(max_lastch, i->lastchpos2);
                        /* passthru */                    
                    case Component::compare_lastch_set:
                    case Component::compare_lastch_boolfunc:
                        max_lastch = std::max(max_lastch, i->lastchpos);
                        break;
                    case Component::compare_boolfunc: ;
                    case Component::compare_endfunc: ;
                }
            }
            for(unsigned s=0; s<subtrees.size(); ++s)
            {
                const RuleTree& p = *subtrees[s];
                max_lastch = std::max(max_lastch, p.CountLastCh());
            }
            return max_lastch;
        }
        
        void GenerateCode(Assembler& Asm, unsigned indent) const
        {
#if 0
            std::cout << "<\n";
#endif
            rule.GenerateCode(Asm, indent);
            
            for(unsigned s=0; s<subtrees.size(); ++s)
            {
                const RuleTree& p = *subtrees[s];
                p.GenerateCode(Asm, indent);
            }
            
            compilation.GenerateCode(Asm, indent);
#if 0
            std::cout << ">\n";
#endif
        }
    };
    
    class Function
    {
        RuleTree rules;

        void GenerateCode(const RuleTree& rules, Assembler& Asm) const
        {
            rules.GenerateCode(Asm, 2);
        }
        
    public:
        void Define(const Rule& rule, const Compilation& compilation)
        {
            RuleTree*r = new RuleTree;
            r->rule        = rule;
            r->compilation = compilation;
            rules.subtrees.push_back(r);
        }
        
        void FindCommonActions(RuleTree::ActionUsageMap& map)
        {
            rules.FindCommonActions(map);
        }
        void ReplaceAllActionByCall(const Compilation& example, const std::wstring& funame)
        {
            rules.ReplaceAllActionByCall(example, funame);
        }
        void ConvertTreeToFunctions(RuleTree::RuleTreeConversionMap& map)
        {
            rules.ConvertTreeToFunctions(map);
        }

        void Optimize()
        {
            rules.Optimize();
        }
        
        void Generate(Assembler& Asm) const
        {
            GenerateCode(rules, Asm);
        }
    };
    
    void BuildFunction(Rule& rule, Compilation& compilation,
                       const std::wstring& mask,
                       const std::wstring& result)
    {
        std::map<wchar_t, unsigned> set_refs;

        /* First find out how many bytes of the mask
         * and the result are common after the star.
         */
        unsigned result_ignore_after_star = 0;
        unsigned result_star_pos = result.find(L'*');
        for(unsigned b=0; b<mask.size(); ++b)
        {
            wchar_t ch = mask[b];
            if(ch == '*') continue;
            
            /* Filters don't eat bytes */
            std::map<wchar_t, std::wstring>::const_iterator filti = filters.find(ch);
            if(filti != filters.end()) continue;
            
            if(result_star_pos != result.npos
            && result_star_pos+1 < result.size()
            && ch == result[result_star_pos+1])
            {
                ++result_ignore_after_star;
                ++result_star_pos;
            }
            else
            {
                result_star_pos = result.npos;
                break;
            }
        }
        
        if(mask.substr(0,1) != L"*")
        {
            fprintf(stderr,
                "ERROR: Can not define function(%s,%s) - must begin with *\n",
                ConvStr(mask).c_str(), ConvStr(result).c_str());
            return;
        }
        
        /* Now, generate the comparison rules. */
        unsigned last_count=0;
        for(unsigned b=mask.size(); b-->0; )
        {
            wchar_t ch = mask[b];
            if(ch == '*') { break; }
            
            /* Filters don't eat bytes */
            std::map<wchar_t, std::wstring>::const_iterator filti = filters.find(ch);
            if(filti != filters.end())
            {
                Component com;
                com.SetBoolFunc(filti->second);
                rule.components.insert(com);
                continue;
            }
            
            std::map<wchar_t, unsigned>::const_iterator refi = set_refs.find(ch);
            if(refi != set_refs.end())
            {
                Component com;
                com.LastEqSet2(last_count, refi->second);
                rule.components.insert(com);
                
                ++last_count;
                continue;
            }
            std::map<wchar_t, std::set<wchar_t> >::const_iterator cseti = charsets.find(ch);
            if(cseti != charsets.end())
            {
#if 0
                std::cout << ConvStr(wformat(L"Charset('%lc'): '%ls'\n", ch, GetCSet(cseti->second).c_str()));
                std::cout << std::flush;
#endif
                
                Component com;
                com.LastBoolFunc(last_count, GetSetFuncName(ch));
                set_refs[ch] = last_count;
                rule.components.insert(com);
                
                ++last_count;
                continue;
            }
            std::wstring tmp; tmp += ch;
            Component com;
            com.LastEqSet(last_count, BuildCharset(tmp));
            rule.components.insert(com);
            
            ++last_count;
        }
        
#if 0
        std::cout << WstrToAsc(wformat(L"# mask(%ls)result(%ls)last(%u)ignore(%u)\n",
                         mask.c_str(), result.c_str(), last_count, result_ignore_after_star));
#endif
        last_count -= result_ignore_after_star;
        
        bool seen_star=false;
        for(unsigned b=0; b<result.size(); ++b)
        {
            wchar_t ch = result[b];
            if(ch == '*')
            {
                seen_star=true;
                Action act;
                act.OutCtx(last_count);
                compilation.actions.push_back(act);
                continue;
            }
            if(seen_star && result_ignore_after_star > 0)
            {
                --result_ignore_after_star;
                continue;
            }
#if 0
        std::cout << WstrToAsc(wformat(L"# > chr(%lc)\n", ch));
#endif
            
            std::map<wchar_t, unsigned>::const_iterator refi = set_refs.find(ch);
            if(refi != set_refs.end())
            {
                Action act;
                act.OutRef(refi->second);
                compilation.actions.push_back(act);
                continue;
            }
            std::map<wchar_t, std::wstring>::const_iterator filti = filters.find(ch);
            if(filti != filters.end())
            {
                Action act;
                act.CallFunc(filti->second);
                compilation.actions.push_back(act);
                continue;
            }
            Action act;
            act.OutCh(ch);
            compilation.actions.push_back(act);
        }
    }
    
public:
    void Parse(const std::vector<std::wstring>& lines)
    {
        std::wstring lo, up;
        
        typedef std::list<std::wstring> funclist;
        typedef std::map<unsigned, funclist> collist;
        
        collist columns;
        
        for(unsigned a=0; a<lines.size(); ++a)
        {
            const std::wstring& line = lines[a];
            if(line.empty()) continue;
            switch(line[0])
            {
                case '>': up = line.substr(1); break; // uppercase charset
                case '<': lo = line.substr(1); break; // lowercase charset
                case ':':
                {
                    if(line.size() < 3) continue;

                    /* Build u2l and l2u (case conversion maps) */
                    for(unsigned b=0; b<up.size() && b<lo.size(); ++b)
                    {
                        u2l[up[b]] = lo[b]; 
                        l2u[lo[b]] = up[b];
                    }

                    wchar_t ch = line[1];
                    switch(line[2])
                    {
                        case '/': filters[ch]  = line.substr(3); break;
                        case '=':
                        {
                            charsets[ch] = BuildCharset(line.substr(3));
                            break;
                        }
                    }
                    break;
                }
                case '#': case ';': continue; //comment
                case '=':
                {
                    /* Find the column positions and function names from the header */
                    for(unsigned b=0; b<line.size(); ++b)
                    {
                        if(line[b]=='=' || line[b]==' ' || line[b]==' ' || line[b]=='|') continue;
                        unsigned begin = b;
                        while(b < line.size() && line[b]!=' ')++b;
                        const std::wstring funame = line.substr(begin, b-begin);
                        columns[begin].push_back(funame);
                    }
                    break;
                }
                default:
                {
                    /* Anything else - assume it's a mask */
                    unsigned barpos = line.find(L'|');
                    if(barpos == line.npos) continue;
                    
                    std::wstring mask = Trim(line.substr(0, barpos));
                    for(collist::const_iterator next, i = columns.begin(); i != columns.end(); i=next)
                    {
                        next = i; ++next;
                        unsigned begin = i->first;
                        unsigned end = line.size();
                        if(next != columns.end()) end = next->first;
                        
                        std::wstring col = Trim(line.substr(begin, end-begin));
                        
                        for(funclist::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
                        {
                            Rule r;
                            Compilation c;
                            
#if 0
                            std::cout << ConvStr(
                                wformat(L"BuildFunction(%ls)Mask(%ls)Result(%ls)\n",
                                        j->c_str(),mask.c_str(),col.c_str()));
                            std::cout << std::flush;
#endif
        
                            BuildFunction(r, c, mask, col);
                            
                            functions[*j].Define(r, c);
                        }
                    }
                }
            }
        }
        
        for(std::map<std::wstring, Function>::iterator
            i = functions.begin(); i != functions.end(); ++i)
        {
            i->second.Optimize();
        }

        RuleTree::ActionUsageMap map;
        for(std::map<std::wstring, Function>::iterator
            i = functions.begin(); i != functions.end(); ++i)
        {
            i->second.FindCommonActions(map);
        }
        
        unsigned counter = 0;
        for(RuleTree::ActionUsageMap::const_iterator
            i = map.begin(); i != map.end(); ++i)
        {
            if(i->second >= 2)
            {
                const std::wstring funame = wformat(L"ActionFun%u", counter++);
                actionmap[funame] = i->first;

                for(std::map<std::wstring, Function>::iterator
                    j = functions.begin(); j != functions.end(); ++j)
                {
                    j->second.ReplaceAllActionByCall(i->first, funame);
                }
            }
        }
        
#if CONVERT_RULETREES
        for(std::map<std::wstring, Function>::iterator
            i = functions.begin(); i != functions.end(); ++i)
        {
            i->second.ConvertTreeToFunctions(ruletreemap);
        }
#endif
    }
    
    void Generate(Assembler& Asm) const
    {
        for(std::map<wchar_t, std::set<wchar_t> >::const_iterator
            i = charsets.begin(); i != charsets.end(); ++i)
        {
            std::wstring name = GetSetFuncName(i->first);
#if DEBUG_TABLECODE
            std::wcout << L"FUNCTION " << name << std::endl;
#endif
            Asm.START_FUNCTION(name);
            Asm.INDENT_LEVEL(0);
            Asm.DECLARE_REGVAR(Asm.MagicVarName);
            Asm.LOAD_VAR(Asm.MagicVarName);
            Asm.SELECT_CASE(GetCSet(i->second), 0);
              Asm.INDENT_LEVEL(2);
              Asm.BOOLEAN_RETURN(true);
            Asm.INDENT_LEVEL(0);
            Asm.BOOLEAN_RETURN(false);
            Asm.END_FUNCTION();
#if DEBUG_TABLECODE
            //std::wcout << L"END FUNCTION\n";
#endif
        }
        
        for(std::map<std::wstring, Compilation>::const_iterator
            i = actionmap.begin(); i != actionmap.end(); ++i)
        {
#if DEBUG_TABLECODE
            std::wcout << L"FUNCTION " << i->first << std::endl;
#endif
            Asm.START_FUNCTION(i->first);
            Asm.INDENT_LEVEL(0);
            
            i->second.GenerateCode(Asm, 2);
            
            Asm.INDENT_LEVEL(0);
            Asm.VOID_RETURN();
            Asm.END_FUNCTION();
#if DEBUG_TABLECODE
            //std::wcout << L"END FUNCTION\n";
#endif
        }        
        
        for(RuleTree::RuleTreeConversionMap::const_iterator
            i = ruletreemap.begin(); i != ruletreemap.end(); ++i)
        {
#if DEBUG_TABLECODE
            std::wcout << L"FUNCTION " << i->second << std::endl;
#endif
            Asm.START_FUNCTION(i->second);
            Asm.DECLARE_REGVAR(Asm.MagicVarName);
            Asm.INDENT_LEVEL(0);
            
            i->first.GenerateCode(Asm, 2);
            
#if CONVERT_RULETREES
            Asm.INDENT_LEVEL(2);
            Emit("sec"); // carry set="no rule found"
#endif
            Asm.INDENT_LEVEL(0);
            Asm.VOID_RETURN();
            Asm.END_FUNCTION();
#if DEBUG_TABLECODE
            //std::wcout << L"END FUNCTION\n";
#endif
        }
        
        for(std::map<std::wstring, Function>::const_iterator
            i = functions.begin(); i != functions.end(); ++i)
        {
#if DEBUG_TABLECODE
            std::wcout << L"FUNCTION " << i->first << std::endl;
#endif
            Asm.START_FUNCTION(i->first);
            Asm.DECLARE_REGVAR(Asm.MagicVarName);
            Asm.INDENT_LEVEL(0);
            
            i->second.Generate(Asm);
            
#if CONVERT_RULETREES
            Asm.INDENT_LEVEL(2);
            Emit("sec"); // carry set="no rule found"
#endif
            Asm.INDENT_LEVEL(0);
            Asm.VOID_RETURN();
            Asm.END_FUNCTION();
#if DEBUG_TABLECODE
            //std::wcout << L"END FUNCTION\n";
#endif
        }
    }
private:
    RuleTree::RuleTreeConversionMap ruletreemap;
    std::map<std::wstring, Compilation> actionmap;
    std::map<wchar_t, wchar_t> u2l, l2u; // case conversions
    std::map<wchar_t, std::set<wchar_t> > charsets;
    std::map<wchar_t, std::wstring> filters;
    std::map<std::wstring, Function> functions;
};


void Compile(FILE *fp)
{
    Assembler Asm;
    TableParser Tables;
    
    std::wstring file;
    
    if(1) // Read file to wstring
    {
        wstringIn conv;
        conv.SetSet(getcharset());
        
        for(;;)
        {
            char Buf[2048];
            if(!fgets(Buf, sizeof Buf, fp))break;
            file += conv.puts(Buf);
        }
    }
    
    for(unsigned a=0; a<file.size(); )
    {
        std::wstring Buf;
        if(1)
        {
            // Get line
            unsigned b=a;
            while(b<file.size() && file[b]!='\n') ++b;
            Buf = file.substr(a, b-a); a = b+1;
        }
        if(Buf.empty()) continue;
        
        unsigned indent=0;
        vector<std::wstring> words;
        
        if(1) // Initialize indent, words
        {
            const wchar_t *s = Buf.data();
            while(*s == L' ') { ++s; ++indent; }

            words = Split(s, L' ', L'\'', true);
        }
        
        if(words.empty())
        {
            fprintf(stderr, "Weird, '%s' is empty line?\n",
                ConvStr(Buf).c_str());
            continue;
        }
        
        if(words[0][0] == '#')
        {
            continue;
        }
        
        Asm.INDENT_LEVEL(indent);
        
        const std::string firstword = WstrToAsc(words[0]);
        
        if(firstword == "TRUE")
        {
            Asm.BOOLEAN_RETURN(true);
        }
        else if(firstword == "FALSE")
        {
            Asm.BOOLEAN_RETURN(false);
        }
        else if(firstword == "RETURN")
        {
            if(words.size() > 1)
            {
                Asm.LOAD_VAR(words[1]);
            }
            Asm.VOID_RETURN();
        }
        else if(firstword == "BEGIN_TABLES")
        {
            std::vector<std::wstring> lines;
            while(a < file.size())
            {
                std::wstring Buf;
                
                // Get line
                unsigned b=a;
                while(b<file.size() && file[b]!='\n') ++b;
                Buf = file.substr(a, b-a); a = b+1;
                
                if(Buf == L"END_TABLES") break;
                lines.push_back(Buf);
            }
            Tables.Parse(lines);
        }
        else if(firstword == "VAR")
        {
            for(unsigned a=1; a<words.size(); ++a)
            {
                Asm.DECLARE_VAR(words[a]);
            }
        }
        else if(firstword == "REG")
        {
            for(unsigned a=1; a<words.size(); ++a)
            {
                Asm.DECLARE_REGVAR(words[a]);
            }
        }
        else if(firstword == "CALL_GET")
        {
            if(words.size() > 3)
            {
                Asm.LOAD_VAR(words[3]);
            }
            Asm.CALL_FUNC(words[1]);
            Asm.STORE_VAR(words[2]);
        }
        else if(firstword == "IF")
        {
            if(words.size() > 2)
            {
                Asm.LOAD_VAR(words[2]);
            }
            Asm.CALL_FUNC(words[1]);
            Asm.COMPARE_BOOL(indent);
        }
        else if(firstword == "CALL")
        {
            if(words.size() > 2)
            {
                Asm.LOAD_VAR(words[2]);
            }
            Asm.CALL_FUNC(words[1]);
        }
        else if(firstword == "INC")
        {
            Asm.INC_VAR(words[1]);
        }
        else if(firstword == "DEC")
        {
            Asm.DEC_VAR(words[1]);
        }
        else if(firstword == "LET")
        {
            Asm.LOAD_VAR(words[2]);
            Asm.STORE_VAR(words[1]);
        }
        else if(firstword == "LOAD_CHAR")
        {
            Asm.LOAD_VAR(words[2], true);
            Asm.LOAD_CHARACTER();
            Asm.STORE_VAR(words[1]);
        }
        else if(firstword == "LOAD_LAST_CHAR")
        {
            Asm.LOAD_VAR(words[2], true);
            Asm.LOAD_LAST_CHAR();
            Asm.STORE_VAR(words[1]);
        }
        else if(firstword == "=")
        {
            if(WstrToAsc(words[2]) == "0")
                Asm.COMPARE_ZERO(words[1], indent);
            else
            {
                // Changing this has no effect in
                // functionality, but may have in code size
                bool lda_param_2 = Asm.IsCached(words[2]);
                
                if(lda_param_2)
                {
                    Asm.LOAD_VAR(words[2]);
                    Asm.COMPARE_EQUAL(words[1], indent);
                }
                else
                {
                    Asm.LOAD_VAR(words[1]);
                    Asm.COMPARE_EQUAL(words[2], indent);
                }
            }
        }
        else if(firstword == "?")
        {
            Asm.LOAD_VAR(words[1]);
            Asm.SELECT_CASE(words[2], indent);
        }
        else if(firstword == "{")
        {
            Asm.START_CHARNAME_LOOP();
        }
        else if(firstword == "}")
        {
            Asm.END_CHARNAME_LOOP();
        }
        else if(firstword == "OUT")
        {
            Asm.LOAD_VAR(words[1], true);
            Asm.OUT_CHARACTER();
        }
        else if(firstword == "FUNCTION")
        {
            Asm.END_FUNCTION();
            Asm.START_FUNCTION(words[1]);
        }
        else
            fprintf(stderr, "  ERROR: What's this? '%s'\n", firstword.c_str());
    }
    Asm.END_FUNCTION();
    Tables.Generate(Asm);
}

int main(int argc, const char *const *argv)
{
    if(argc != 3)
    {
        fprintf(stderr, "Error: Wrong usage.\n"
                        "Correct usage:\n"
                        " utils/compiler <codefile> <asmfile>\n"
                        "This program reads <codefile> and produces <asmfile>.\n"
               );
        return -1;
    }
    FILE *fp = fopen(argv[1], "rt"); if(!fp) { perror(argv[1]); return -1; }
    FILE *fo = fopen(argv[2], "wt"); if(!fo) { perror(argv[2]); return -1; }
    
    BeginCode(fo);
    
    Compile(fp);
    
    EndCode();
    
    fclose(fp);
    fclose(fo);
    
    return 0;
}

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
                Emit(string("nop;")+prefix+"carry insignificant");
            else if(state == CarrySet)
                Emit(string("nop;")+prefix+"carry set");
            else if(state == CarryUnset)
                Emit(string("nop;")+prefix+"carry unset");
            else
                Emit(string("nop;")+prefix+"carry unknown");
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
        ucs4string CurSubName;
        
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
        
        typedef map<ucs4string, variable> vars_t;
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
        typedef list<BranchData> branchlist_t;
        branchlist_t openbranches;
        
        struct FunctionData
        {
            FlagAssumption flags;
            unsigned    varcount;
        public:
            FunctionData(): flags(), varcount(0) { }
        };
        typedef map<ucs4string, FunctionData> functionlist_t;
        functionlist_t functiondata;
        
        ucs4string PendingCall;
        
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
            ucs4string var;
        public:
            VarState() : known(false) {}
            void Invalidate() { known=false; }
            bool Known() const { return known; }
            void Set(const ucs4string &v) { known=true; var=v; }
            bool Is(const ucs4string &v) const { return known && var==v; }
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
            
            CLEARSTR(PendingCall);
        }
        void CheckCodeStart()
        {
            CheckPendingCall();
            if(started) return;
            
            FRAME_BEGIN();
            started = true;
        }
        unsigned char GetStackOffset(const ucs4string &varname) const
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
        const ucs4string LoopHelperName;
        const ucs4string OutcHelperName;
        const ucs4string MagicVarName;
        
        Assembler()
        : CurSubName(),
          vars(),
          started(),
          LoopCount(),
          openbranches(),
          functiondata(),
          PendingCall(),
          ALstate(), AHstate(),
          LoopStack(),
          LoopHelperName(GetConf("compiler", "loophelpername")),
          OutcHelperName(GetConf("compiler", "outchelpername")),
          MagicVarName(GetConf("compiler", "magicvarname"))
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
        void START_FUNCTION(const ucs4string &name)
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
        void DECLARE_VAR(const ucs4string &name)
        {
            if(vars.find(name) == vars.end())
            {
                // pino-osoitteet pienenevät pushatessa.
                //    s = 6E5
                //   pha - this goes to 6E5
                //   pha - this goes to 6E4
                //   pha - this goes to 6E3
                // tämän jälkeen s = 6E2.
                // Thus, [s+1] refers to var1, [s+2] to var2, and so on.
                
                unsigned stackpos = vars.size() + 1;
                
                vars[name].is_regvar = false;
                vars[name].stackpos = stackpos;
            }
        }
        void DECLARE_REGVAR(const ucs4string &name)
        {
            vars[name].is_regvar = true;
        }
        void VOID_RETURN()
        {
            if(!PendingCall.empty())
            {
                std::string Target = WstrToAsc(PendingCall);
                CLEARSTR(PendingCall);
                CheckCodeStart();
                DeallocateVars();
                
                EmitBranch("bra", Target);
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
        
        bool IsCached(const ucs4string &name) const
        {
            return ALstate.Var.Is(name);
        }
        
        void LOAD_VAR(const ucs4string &name, bool need_16bit = false)
        {
            CheckCodeStart();
            // load var to A
            
            if(vars.find(name) != vars.end())
            {
                vars[name].read = true;

                if(ALstate.Var.Is(name)) return;

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
                val = getchronochar(name[1], cset_12pix);
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
        void STORE_VAR(const ucs4string &name)
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
        void INC_VAR(const ucs4string &name)
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
        void DEC_VAR(const ucs4string &name)
        {
            CheckCodeStart();
            // dec var
            LOAD_VAR(name);
            Emit("dec"); // A bitness doesn't really matter here

            ALstate.Var.Invalidate();
            ALstate.Const.Dec();

            STORE_VAR(name);
        }
        void CALL_FUNC(const ucs4string &name)
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
        void COMPARE_EQUAL(const ucs4string &name, unsigned indent)
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
                    val = getchronochar(name[1], cset_12pix);
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
        void COMPARE_ZERO(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if var is zero
            LOAD_VAR(name);

            // Note: we're not checking ALstate here, would be mostly useless check
            
            GenerateComparison(indent, "bne", "beq");
        }
        void SELECT_CASE(const ucs4string &cset, unsigned indent)
        {
            class CaseHandler: public CaseGenerator
            {
                bool      LastWasBra;
                bool      ChainCompare;
                CaseValue LastCompare;
                bool      BraPending;
                string    PendingLabel;
                string    LastCompareType;
                
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
                
                const FlagAssumption& GetFlags(const string& label)
                {
                    return jumps[label].flags;
                }
                
                const CarryAssumption& GetCarry(const string& label)
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
                
                ctchar c = getchronochar(cset[a], cset_12pix);
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
        void OUT_CHARACTER()
        {
            // outputs character in A
            CALL_FUNC(OutcHelperName);
        }
    };
}

void Compile(FILE *fp)
{
    Assembler Asm;
    
    ucs4string file;
    
    if(1) // Read file to ucs4string
    {
        wstringIn conv(getcharset());
        
        for(;;)
        {
            char Buf[2048];
            if(!fgets(Buf, sizeof Buf, fp))break;
            file += conv.puts(Buf);
        }
    }
    
    for(unsigned a=0; a<file.size(); )
    {
        ucs4string Buf;
        if(1)
        {
            // Get line
            unsigned b=a;
            while(b<file.size() && file[b]!='\n') ++b;
            Buf = file.substr(a, b-a); a = b+1;
        }
        if(Buf.empty()) continue;
        
        unsigned indent=0;
        vector<ucs4string> words;
        
        if(1) // Initialize indent, words
        {
            const ucs4 *s = Buf.data();
            while(*s == ' ') { ++s; ++indent; }

            ucs4string rest = s;
            for(;;)
            {
                unsigned spacepos = rest.find(' ');
                if(spacepos == rest.npos)break;
                words.push_back(rest.substr(0, spacepos));
                if(spacepos+1 >= rest.size()) { CLEARSTR(rest); break; }
                rest = rest.substr(spacepos+1);
            }
            if(!rest.empty()) { words.push_back(rest); CLEARSTR(rest); }
        }
        
        if(words.empty())
        {
            fprintf(stderr, "Weird, '%s' is empty line?\n",
                WstrToAsc(Buf).c_str());
            continue;
        }
        
        if(words[0][0] == '#')
        {
            continue;
        }
        
        Asm.INDENT_LEVEL(indent);
        
        const string firstword = WstrToAsc(words[0]);
        
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

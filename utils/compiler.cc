#include <cstdio>
#include <set>
#include <map>
#include <list>
#include <vector>

#include "wstring.hh"
#include "config.hh"
#include "ctcset.hh"

using namespace std;

#define SIMPLE_OUT_FORMAT 0

namespace
{
    FILE *OutFile;
    
    class Assembler
    {
        ucs4string CurSubName;
        
        struct variable
        {
            unsigned stackpos;
            bool read,written,loaded;
            bool is_regvar;
            
            variable() : stackpos(0),read(false),written(false),loaded(false)
            {
                is_regvar = false;
            }
        };
        
        typedef map<ucs4string, variable> vars_t;
        vars_t vars;

        bool started;       // Code begun?
        unsigned LoopCount; // How many loops active
        
        bool end_defined;
        bool end_needed;
        
        typedef list<pair<unsigned, const char *> > branchlist_t;
        branchlist_t openbranches;
        
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
            
            void Invalidate() { Const.Invalidate(); Var.Invalidate(); }
        };
        
        regstate ALstate, AHstate;
        
        void Invalidate_A()
        {
            ALstate.Invalidate();
            AHstate.Invalidate();
        }
        
        void DeallocateVars()
        {
            // Count variables
            unsigned varcount = vars.size();
            // But don't count register variables
            for(vars_t::const_iterator i=vars.begin(); i!=vars.end(); ++i)
                if(i->second.is_regvar)
                    --varcount;
            
            if(varcount != 3 || X_is_16bit())
            {
                if(!X_is_8bit() || varcount > 4)
                {
                    while(varcount >= 2)
                    {
                        Emit_X(16);
                        Emit("ply");
                        varcount -= 2;
                    }
                }
            }
            while(varcount >= 1)
            {
                Emit_X(8);
                Emit("ply");
                varcount -= 1;
            }
        }
        
        bool IsPlainRTS() const
        {
            // Count variables
            unsigned varcount = vars.size();
            // But don't count register variables
            for(vars_t::const_iterator i=vars.begin(); i!=vars.end(); ++i)
                if(i->second.is_regvar)
                    --varcount;

            return !varcount;
        }
        
        void FRAME_BEGIN()
        {
            // Count variables
            unsigned varcount = vars.size();
            // But don't count register variables
            for(vars_t::const_iterator i=vars.begin(); i!=vars.end(); ++i)
                if(i->second.is_regvar)
                    --varcount;
            
            /* Note: All vars will be initialized with A's current value */
        #if 0
            // That's why this is disabled
            while(varcount >= 2)
            {
                Emit_M(16);
                Emit("pha");
                varcount -= 2;
            }
        #endif
            while(varcount >= 1)
            {
                Emit_M(8);
                Emit("pha");
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
        void CheckCodeStart()
        {
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
        
        void EmitStack(const char* op, unsigned index)
        {
            char Buf[64];
            sprintf(Buf, "%.5s $%u,s", op, index);;
            Emit(Buf);
        }
        void EmitImmed(const char* op, int value)
        {
            char Buf[64];
            sprintf(Buf, "%.5s #$%X", op, value);
            Emit(Buf);
        }
        void EmitParam(const char* op, const char* param)
        {
            string s = op;
            s += ' ';
            s += param;
            Emit(s.c_str());
        }
        void EmitCall(const ucs4string& name)
        {
            string tmp = "jsr ";
            tmp += WstrToAsc(name);
            Emit(tmp.c_str());
            EmitUnknownBits();
            EmitAnyBits();
        }
        void EmitFunction(const ucs4string& name)
        {
            string tmp = WstrToAsc(name);
            tmp += ':';
            Emit(tmp.c_str());
            EmitUnknownBits();
            EmitAnyBits();
        }

        enum { StateUnknown, State16, State8, StateAnything }
            StateX, StateM,
            WantedX, WantedM;
#if !SIMPLE_OUT_FORMAT
        bool newline;
        unsigned indent;
#endif

        void Flushbits()
        {
            unsigned rep = 0;
            unsigned sep = 0;
            if(WantedX != StateAnything && WantedX != StateX)
            {
                if(WantedX == State16) rep |= 0x10; else sep |= 0x10;
                StateX = WantedX;
            }
            if(WantedM != StateAnything && WantedM != StateM)
            {
                if(WantedM == State16) rep |= 0x20; else sep |= 0x20;
                StateM = WantedM;
            }
            /*static int Xflag=0, Aflag=0;*/
            if(sep & 0x10) { /*if(Xflag!=8){Xflag=8;*/Emit(".xs");/*}*/ EmitNoNewline(); }
            if(sep & 0x20) { /*if(Aflag!=8){Aflag=8;*/Emit(".as");/*}*/ EmitNoNewline(); }
            if(rep & 0x10) { /*if(Xflag!=16){Xflag=16;*/Emit(".xl");/*}*/ EmitNoNewline(); }
            if(rep & 0x20) { /*if(Aflag!=16){Aflag=16;*/Emit(".al");/*}*/ EmitNoNewline(); }
            if(sep)EmitImmed("sep", sep);
            if(rep)EmitImmed("rep", rep);
        }
        bool A_is_8bit() const { return StateM == State8; }
        bool X_is_8bit() const { return StateX == State8; }
        bool A_is_16bit() const { return StateM == State16; }
        bool X_is_16bit() const { return StateX == State16; }
        
        void Emit_M(unsigned bitness)
        {
            WantedM = bitness==8 ? State8 : State16;
        }
        void Emit_X(unsigned bitness)
        {
            WantedX = bitness==8 ? State8 : State16;
        }
        void EmitAnyBits()
        {
            WantedX = WantedM = StateAnything;
            EmitUnknownBits();
        }
        void EmitUnknownBits()
        {
            StateX = StateM = StateUnknown;
        }
        void EmitLabel(const char* name)
        {
            string tmp = name;
            tmp += ':';
            Emit(tmp.c_str());
            EmitUnknownBits();
            EmitAnyBits();
        }
        void Emit(const char* code)
        {
            Flushbits();
            
#if !SIMPLE_OUT_FORMAT
            if(!strcmp(code, ".)")) if(indent > 0) --indent;

            if(newline) fputc('\n', OutFile);
            else fprintf(OutFile, " : ");
            if(*code != '+' && newline)
            {
                for(unsigned a=0; a<indent; ++a)
                    fputc('\t', OutFile);
            }
#endif
            fprintf(OutFile, "%s", code);
#if !SIMPLE_OUT_FORMAT
            newline = true;
            
            if(*code == 'c' && code[1] == 'm') newline = false;
            if(strchr(code, ';')) newline = true;

            if(!strcmp(code, ".(")) ++indent;
#else
            fputc('\n', OutFile);
#endif
        }
        const char *GenLabel() const
        {
            static list<string> labels;
            char Buf[64];
            sprintf(Buf, "L%u", 1+labels.size());
            labels.push_front(Buf);
            return labels.begin()->c_str();
        }

        void AddBranch(const char* name, unsigned ind)
        {
            ++indent;
            openbranches.push_back(make_pair(ind, name));
        }
        
        void GenerateComparison(const char* notjump, const char* jump, bool SHORT=true)
        {
            if(SHORT)
            {
                const char *elselabel = GenLabel();
                EmitParam(notjump, elselabel);
                //EmitAnyBits(); 
                AddBranch(elselabel, indent);
            }
            else
            {
                const char *shortlabel = GenLabel();
                const char *elselabel = GenLabel();
                
                EmitParam(jump, shortlabel);
                EmitParam("brl", elselabel);
                EmitLabel(shortlabel);
                //EmitAnyBits();
                AddBranch(elselabel, indent);
            }
        }
    public:
        void EmitNoNewline()
        {
#if !SIMPLE_OUT_FORMAT
            newline = false;
#endif
        }
        
        const ucs4string LoopHelperName;
        const ucs4string OutcHelperName;
        const ucs4string MagicVarName;
        
        Assembler()
        : LoopHelperName(GetConf("compiler", "loophelpername")),
          OutcHelperName(GetConf("compiler", "outchelpername")),
          MagicVarName(GetConf("compiler", "magicvarname"))
        {
            StateX=StateM=WantedX=WantedM=StateAnything;
            LoopCount=0;
#if !SIMPLE_OUT_FORMAT
            newline = true;
            indent = 0;
#endif
        }
        
        void BRANCH_LEVEL(unsigned level)
        {
            // Go the label list in REVERSE order; First-in, last-out
            for(branchlist_t::reverse_iterator
                a = openbranches.rbegin();
                a != openbranches.rend();
                ++a)
            {
                if(a->first != 999 && level <= a->first)
                {
                    Invalidate_A();

                    --indent;
                    EmitLabel(a->second);
                    //Emit(".)");
                    a->first = 999; // prevent being reassigned
                }
            }
        }
        void START_FUNCTION(const ucs4string &name)
        {
            CurSubName = name;
            started = false;
            vars.clear();
            
            string def = "#ifndef OMIT_" + WstrToAsc(name);
            Emit(def.c_str());

            EmitFunction(name);
            Emit(".(");
            
            Invalidate_A();
            
            end_needed  = false;
            end_defined = false;
        }
        void END_FUNCTION()
        {
            CheckCodeStart();
            FRAME_END();
            if(!CurSubName.empty())
            {
                Emit(".)");
                Emit("#endif");
#if !SIMPLE_OUT_FORMAT
                Emit("\n");
#endif
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
            CheckCodeStart();
            
            bool inline_end = IsPlainRTS();
            if(!end_defined) inline_end = true;
            
            if(inline_end)
            {
                if(!end_defined)
                {
                    EmitLabel("End");
                    end_defined = true;
                }

                DeallocateVars();
                Emit("rts");
            }
            else
            {
                end_needed = true;
                EmitParam("bra", "End");
            }
        }
        void BOOLEAN_RETURN(bool value)
        {
            CheckCodeStart();
            // emit flag, then return
            
            // CLC (nonset) = false, SEC (set) = true
            Emit(value ? "sec" : "clc");
            VOID_RETURN();
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
                            Emit_M(16);
                            Emit("lda #0");
                            ALstate.Const.Set(0);
                            ALstate.Var.Invalidate();
                            AHstate.Const.Set(0);
                            AHstate.Var.Invalidate();
                        }
                        else
                        {
                            Emit_M(8);
                            Emit("lda #0");
                            Emit("xba");
                            ALstate = AHstate; // AL has now what was in AH
                            AHstate.Const.Set(0); // And AH is 0 as we wanted
                            AHstate.Var.Invalidate();
                        }
                    }
                }
                
                Emit_M(8);
                EmitStack("lda", stackpos);

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
            
            if(need_16bit || A_is_16bit())
            {
                if(A_is_16bit() || !AHstate.Const.Is(hi))
                {
                    Emit_M(16);
                    EmitImmed("lda", val);
                    ALstate.Const.Set(lo);
                    ALstate.Var.Invalidate();
                    AHstate.Const.Set(hi);
                    AHstate.Var.Invalidate();
                    return;
                }
            }
            if(!ALstate.Const.Is(lo))
            {
                Emit_M(8);
                EmitImmed("lda", lo);
                ALstate.Const.Set(lo);
                ALstate.Var.Invalidate();
            }
        }
        void STORE_VAR(const ucs4string &name)
        {
            if(ALstate.Var.Is(name)) return;

            if(vars.find(name) == vars.end()
            || !vars.find(name)->second.is_regvar)
            {
                CheckCodeStart();
                // store A to var
                Emit_M(8);
                unsigned stackpos = GetStackOffset(name);
                EmitStack("sta", stackpos);
            }
            vars[name].written = true;
            
            ALstate.Var.Set(name);
        }
        void INC_VAR(const ucs4string &name)
        {
            CheckCodeStart();
            // inc var
            LOAD_VAR(name);
            EmitNoNewline();
            Emit_M(8);
            Emit("inc");

            ALstate.Var.Invalidate();
            ALstate.Const.Inc();

            STORE_VAR(name);
        }
        void DEC_VAR(const ucs4string &name)
        {
            CheckCodeStart();
            // dec var
            LOAD_VAR(name);
            EmitNoNewline();
            Emit_M(8);
            Emit("dec");

            ALstate.Var.Invalidate();
            ALstate.Const.Dec();

            STORE_VAR(name);
        }
        void CALL_FUNC(const ucs4string &name)
        {
            CheckCodeStart();
            
#if !SIMPLE_OUT_FORMAT
            newline = true;
#endif
            // X needs to be saved if we're in a loop.
            if(LoopCount)
            {
                Emit_X(16);
                Emit("phx");
            }
            
            // leave A unmodified and issue call to function
            EmitCall(name);
            
            if(LoopCount)
            {
                Emit_X(16);
                Emit("plx");
            }

            Invalidate_A();
        }
        void COMPARE_BOOL(unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if boolean set
            
            GenerateComparison("bcc", "bcs");
        }
        void COMPARE_EQUAL(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();
            
            if(vars.find(name) != vars.end())
            {
                unsigned stackpos = GetStackOffset(name);
                vars[name].read = vars[name].loaded = true;
                
                Emit_M(8);
                
                // process subblock if A is equal to given var
                // Note: we're not checking ALstate here, would be mostly useless check
                EmitStack("cmp", stackpos);
            }
            else
            {
                unsigned val = 0;
                
                if(name[0] == '\'')
                    val = getchronochar(name[1], cset_12pix);
                else
                    val = strtol(WstrToAsc(name).c_str(), NULL, 10);
                
                if(A_is_8bit() && val >= 256)
                {
                    fprintf(stderr,
                        "  Warning: In function '%s', numeric constant %u too large (>255)\n",
                        WstrToAsc(CurSubName).c_str(), val);
                }
                // bitness is insignificant
                EmitImmed("cmp", val);
            }
            
            GenerateComparison("bne", "beq");
        }
        void COMPARE_ZERO(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if var is zero
            LOAD_VAR(name);
            EmitNoNewline();

            // Note: we're not checking ALstate here, would be mostly useless check

            GenerateComparison("bne", "beq");
        }
        void COMPARE_GREATER(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();

            unsigned stackpos = GetStackOffset(name);
            vars[name].read = vars[name].loaded = true;

            Emit_M(8);
            
            // process subblock if A is greater than given var
            EmitStack("cmp", stackpos);
            
            const char *shortlabel1 = GenLabel();
            const char *shortlabel2 = GenLabel();
            const char *elselabel = GenLabel();
            EmitParam("bcs", shortlabel1); // BCS- Jump to if greater or equal
            EmitParam("brl", elselabel);   // BRL- Jump to "else"
            EmitLabel(shortlabel1);
            //EmitAnyBits();
            EmitParam("bne", shortlabel2); // BNE- Jump to n-eq too (leaving only "greater")
            EmitParam("brl", elselabel);   // BRL- Jump to "else"
            EmitLabel(shortlabel2);
            //EmitAnyBits();
            AddBranch(elselabel, indent);
        }
        void SELECT_CASE(const ucs4string &cset, unsigned indent)
        {
            if(cset.empty()) return;
            
            CheckCodeStart();

            // process subblock if A is in any of given chars
            
            vector<ctchar> allowed;
            
            for(unsigned a=0; a<cset.size(); ++a)
            {
                // Names can only contain 8bit chars!
                // Note: we're not checking ALstate here, would be mostly useless check
                
                ctchar c = getchronochar(cset[a], cset_12pix);
                allowed.push_back(c);
            }
            sort(allowed.begin(), allowed.end());
            vector<unsigned> rangebegins, rangeends;
            
            const ctchar* begin = &*allowed.begin();
            const ctchar* end   = &*allowed.end();
            ctchar first=0, last=0; bool eka=true;
            while(begin < end)
            {
                if(eka) { eka=false; NewRange: first=last=*begin++; continue; }
                if(*begin == last+1) { last=*begin++; continue; }
                
                rangebegins.push_back(first); rangeends.push_back(last);
                goto NewRange;
            }
            rangebegins.push_back(first); rangeends.push_back(last);
            bool used_next = false;

            const char *shortlabel = GenLabel();
            const char *elselabel = GenLabel();

            for(unsigned a=0; a<rangebegins.size(); ++a)
            {
                const bool last = (a+1) == rangebegins.size();
                const ctchar c1 = rangebegins[a];
                const ctchar c2 = rangeends[a];
                
                // Input characters are always 8-bit. (The output is 16-bit)
                Emit_M(8);
                if(last)
                {
                    if(c1 == c2)
                    {
                        EmitImmed("cmp", c1); EmitParam("bne", elselabel);
                    }
                    else if(c1+1 == c2)
                    {
                        used_next = true;
                        EmitImmed("cmp", c1); EmitParam("beq", shortlabel);
                        EmitImmed("cmp", c2); EmitParam("bne", elselabel);
                    }
                    else
                    {
                        EmitImmed("cmp", c1);   EmitParam("bcc", elselabel);
                        if(c2 < 0xFF)
                        {
                            EmitImmed("cmp", c2+1); EmitParam("bcs", elselabel);
                        }
                    }
                }
                else
                {
                    if(c1 == c2)
                    {
                        used_next = true;
                        EmitImmed("cmp", c1); EmitParam("beq", shortlabel);
                    }
                    else if(c1+1 == c2)
                    {
                        used_next = true;
                        EmitImmed("cmp", c1); EmitParam("beq", shortlabel);
                        EmitImmed("cmp", c2); EmitParam("beq", shortlabel);
                    }
                    else
                    {
                        EmitImmed("cmp", c1);   EmitParam("bcc", elselabel);
                        if(c2 >= 0xFF)
                        {
                            used_next = true;
                            EmitParam("bra", shortlabel);
                        }
                        else
                        {
                            used_next = true;
                            EmitImmed("cmp", c2+1); EmitParam("bcc", shortlabel);
                        }
                    }
                }
            }
            if(used_next) EmitLabel(shortlabel);
            //EmitAnyBits();
            AddBranch(elselabel, indent);
        }
        void START_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            ++LoopCount;
            Emit(".(");
            Emit("; Init loop");
            Emit_M(16);
            Emit_X(16);
            Emit("ldx #0");
            EmitLabel("LoopBegin");
            EmitCall(LoopHelperName);
            Emit("; If zero, break the loop");
            EmitParam("beq", "LoopEnd");

            Invalidate_A();

            STORE_VAR(MagicVarName);// save in "c".
            // mark read, because it indeed has been
            // read (in the loop end condition).
            vars[MagicVarName].read = true;
        }
        void END_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            Emit_X(16);
            Emit("inx");
            EmitParam("bra", "LoopBegin");
            EmitLabel("LoopEnd");

            Invalidate_A();
            
            Emit(".)");
            --LoopCount;
        }
        void OUT_CHARACTER()
        {
            CheckCodeStart();
            // outputs character in A
            
            EmitCall(OutcHelperName);

            Invalidate_A();
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
        
        Asm.BRANCH_LEVEL(indent);
        
        if(words[0][0] == '#')
        {
#if !SIMPLE_OUT_FORMAT
            string s = "; ";
            s += WstrToAsc(Buf).c_str() + 1;
            fprintf(OutFile, "%s\n", s.c_str());
#endif
            continue;
        }
        
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
                Asm.EmitNoNewline();
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
            Asm.CALL_FUNC(words[1]);
            Asm.EmitNoNewline();
            Asm.STORE_VAR(words[2]);
        }
        else if(firstword == "IF")
        {
            if(words.size() > 2)
            {
                Asm.LOAD_VAR(words[2]);
                Asm.EmitNoNewline();
            }
            Asm.CALL_FUNC(words[1]);
            Asm.COMPARE_BOOL(indent);
        }
        else if(firstword == "CALL")
        {
            if(words.size() > 2)
            {
                Asm.LOAD_VAR(words[2]);
                Asm.EmitNoNewline();
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
            Asm.EmitNoNewline();
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
                    Asm.EmitNoNewline();
                    Asm.COMPARE_EQUAL(words[1], indent);
                }
                else
                {
                    Asm.LOAD_VAR(words[1]);
                    Asm.EmitNoNewline();
                    Asm.COMPARE_EQUAL(words[2], indent);
                }
            }
        }
        else if(firstword == ">")
        {
            Asm.LOAD_VAR(words[2]);
            Asm.EmitNoNewline();
            Asm.COMPARE_GREATER(words[1], indent);
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
            Asm.EmitNoNewline();
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
    
    OutFile = fo;
    
    Compile(fp);
    OutFile = NULL;
    
    fclose(fp);
    fclose(fo);
    
    return 0;
}

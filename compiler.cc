#include <set>
#include <map>

#include "compiler.hh"
#include "wstring.hh"
#include "config.hh"
#include "ctcset.hh"

using namespace std;

namespace
{
    class Assembler
    {
        SubRoutine *cursub;
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

        // code begun?
        bool started;
        
        typedef SNEScode::RelativeLongBranch Branch;
        
        Branch *loopbegin;
        Branch *loopend;
        Branch *endbranch;

        #define CODE cursub->code
        
        FunctionList functions;

        typedef list<pair<unsigned, Branch> > branchlist_t;
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
        
        void AddBranch(const Branch &b, unsigned ind)
        {
            openbranches.push_back(make_pair(ind, b));
        }
        
        void FRAME_BEGIN()
        {
            unsigned varcount = vars.size();
            /* Note: All vars will be initialized with A's current value */
        #if 0
            if(varcount >= 2) CODE.Set16bit_M();
            while(varcount >= 2)
            {
                CODE.EmitCode(0x48); // PHA
                varcount -= 2;
            }
        #endif
            if(varcount >= 1) CODE.Set8bit_M();
            while(varcount >= 1)
            {
                CODE.EmitCode(0x48); // PHA
                varcount -= 1;
            }
            endbranch = new Branch(CODE.PrepareRelativeLongBranch());

            Invalidate_A();
            ALstate.Var.Set(MagicVarName);
            ALstate.Const.Invalidate();
        }
        void FRAME_END()
        {
            endbranch->ToHere();
            endbranch->Proceed();
            delete endbranch; endbranch = NULL;
            
            unsigned varcount = vars.size();
            if(varcount >= 2) CODE.Set16bit_X();
            while(varcount >= 2)
            {
                CODE.EmitCode(0x7A); // PLY
                varcount -= 2;
            }
            if(varcount >= 1) CODE.Set8bit_X();
            while(varcount >= 1)
            {
                CODE.EmitCode(0x7A); // PLY
                varcount -= 1;
            }
            
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
            CODE.EmitCode(0x6B);     // RTL
#if 0
            fprintf(stderr, "Function %s done:\n", WstrToAsc(CurSubName).c_str());
            fprintf(stderr, "Code:");
            for(unsigned a=0; a<CODE.size(); ++a)
                fprintf(stderr, " %02X", CODE[a]);
            fprintf(stderr, "\n");
#endif
        }
        void FINISH_BRANCHES()
        {
            for(branchlist_t::iterator
                a=openbranches.begin();
                a!=openbranches.end();
                ++a)
            {
                a->second.Proceed();
            }
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

    public:
        const ucs4string LoopHelperName;
        const ucs4string OutcHelperName;
        const ucs4string MagicVarName;
        
        Assembler()
        : cursub(NULL),
          LoopHelperName(GetConf("compiler", "loophelpername")),
          OutcHelperName(GetConf("compiler", "outchelpername")),
          MagicVarName(GetConf("compiler", "magicvarname"))
        {
        }
        
        void BRANCH_LEVEL(unsigned indent)
        {
            if(!cursub) return;
            for(branchlist_t::iterator
                a=openbranches.begin();
                a!=openbranches.end();
                ++a)
            {
                if(a->first != 999 && indent <= a->first)
                {
                    Invalidate_A();

                    a->second.ToHere();
                    a->first = 999; // prevent being reassigned
                }
            }
        }
        void START_FUNCTION(const ucs4string &name)
        {
            CurSubName = name;
            started = false;
            vars.clear();

            cursub = new SubRoutine;
            
            Invalidate_A();
        }
        void END_FUNCTION()
        {
            if(!cursub) return;
            CheckCodeStart();
            FRAME_END();
            
            functions.Define(CurSubName, *cursub);
            
            delete cursub;
            cursub = NULL;
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

            CODE.EmitCode(0x82, 0,0); // BRL- Jump to end
            endbranch->FromHere();
            CODE.BitnessAnything();
        }
        void BOOLEAN_RETURN(bool value)
        {
            CheckCodeStart();
            // emit flag, then return
            if(value)
                CODE.EmitCode(0x18); // CLC - carry nonset = true
            else
                CODE.EmitCode(0x38); // SEC - carry set = false
            VOID_RETURN();
        }
        void LOAD_VAR(const ucs4string &name)
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
                
                if(!AHstate.Const.Is(0))
                {
                    if(!ALstate.Const.Is(0))
                    {
                        CODE.Set8bit_M();
                        CODE.EmitCode(0xA9, 0);
                        AHstate.Const.Set(0);
                        AHstate.Var.Invalidate();
                    }
                    CODE.EmitCode(0xEB); // XBA
                    AHstate = ALstate;
                }
                
                CODE.Set8bit_M();
                CODE.EmitCode(0xA3, stackpos); // LDA [00:s+n]

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
            
            if(!AHstate.Const.Is(hi))
            {
                CODE.Set16bit_M();
                CODE.EmitCode(0xA9, val&255, val>>8); // LDA A, imm16
                ALstate.Const.Set(lo);
                ALstate.Var.Invalidate();
                AHstate.Const.Set(hi);
                AHstate.Var.Invalidate();
                return;
            }
            
            if(!ALstate.Const.Is(lo))
            {
                CODE.Set8bit_M();
                CODE.EmitCode(0xA9, lo); // LDA A, imm8
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
                CODE.Set8bit_M();
                unsigned stackpos = GetStackOffset(name);
                CODE.EmitCode(0x83, stackpos); // STA [00:s+n]
            }
            vars[name].written = true;
            
            ALstate.Var.Set(name);
        }
        void INC_VAR(const ucs4string &name)
        {
            CheckCodeStart();
            // inc var
            LOAD_VAR(name);
            CODE.Set8bit_M();
            CODE.EmitCode(0x1A); // INC A

            ALstate.Var.Invalidate();
            ALstate.Const.Inc();

            STORE_VAR(name);
        }
        void DEC_VAR(const ucs4string &name)
        {
            CheckCodeStart();
            // dec var
            LOAD_VAR(name);
            CODE.Set8bit_M();
            CODE.EmitCode(0x3A); // DEC A

            ALstate.Var.Invalidate();
            ALstate.Const.Dec();

            STORE_VAR(name);
        }
        void CALL_FUNC(const ucs4string &name)
        {
            CheckCodeStart();
            
            CODE.Set16bit_X();
            CODE.EmitCode(0xDA); // PHX
            
            // leave A unmodified and issue call to function
            cursub->CallSub(name);
            
            CODE.Set16bit_X();
            CODE.EmitCode(0xFA); // PLX

            Invalidate_A();
        }
        void COMPARE_BOOL(unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if boolean set
            
            Branch b = CODE.PrepareRelativeLongBranch();
            CODE.EmitCode(0x90, 3);   // BCC- Jump to if true
            CODE.EmitCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            CODE.BitnessAnything();
            AddBranch(b, indent);
        }
        void COMPARE_EQUAL(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();
            
            unsigned stackpos = GetStackOffset(name);
            vars[name].read = vars[name].loaded = true;
            
            CODE.Set8bit_M();
            
            // process subblock if A is equal to given var
            // Note: we're not checking ALstate here, would be mostly useless check
            CODE.EmitCode(0xC3, stackpos); // CMP [00:s+n]

            Branch b = CODE.PrepareRelativeLongBranch();
            CODE.EmitCode(0xF0, 3);   // BEQ- Jump to if eq
            CODE.EmitCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            CODE.BitnessAnything();
            AddBranch(b, indent);
        }
        void COMPARE_ZERO(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if var is zero
            LOAD_VAR(name);

            // Note: we're not checking ALstate here, would be mostly useless check

            Branch b = CODE.PrepareRelativeLongBranch();
            CODE.EmitCode(0xF0, 3);   // BEQ- Jump to if zero
            CODE.EmitCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            CODE.BitnessAnything();
            AddBranch(b, indent);
        }
        void COMPARE_GREATER(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();

            unsigned stackpos = GetStackOffset(name);
            vars[name].read = vars[name].loaded = true;

            CODE.Set8bit_M();
            
            // process subblock if A is greater than given var
            CODE.EmitCode(0xC3, stackpos); // CMP [00:s+n]
            
            Branch b = CODE.PrepareRelativeLongBranch();
            CODE.EmitCode(0xB0, 3);   // BCS- Jump to if greater or equal
            CODE.EmitCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            CODE.BitnessAnything();
            CODE.EmitCode(0xD0, 3);   // BCS- Jump to n-eq too (leaving only "greater")
            CODE.EmitCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            CODE.BitnessAnything();
            
            AddBranch(b, indent);
        }
        void SELECT_CASE(const ucs4string &cset, unsigned indent)
        {
            if(cset.empty()) return;
            
            CheckCodeStart();

            // process subblock if A is in any of given chars
            Branch b = CODE.PrepareRelativeLongBranch();
            
            SNEScode::RelativeBranch into = CODE.PrepareRelativeBranch();
            for(unsigned a=0; a<cset.size(); ++a)
            {
                // FIXME: Ensure we won't mess up with extrachars here.
                // Names can only contain 8bit chars!

                // Note: we're not checking ALstate here, would be mostly useless check
                
                ctchar c = getchronochar(cset[a], cset_12pix);
                CODE.Set8bit_M();
                CODE.EmitCode(0xC9, c);   // CMP A, imm8            
                CODE.EmitCode(0xF0, 0);   // BEQ - Jump to if equal
                into.FromHere();
                
                if(a+1 == cset.size())
                {
                    // If this was the last comparison, do the "else" here.
                    CODE.EmitCode(0x82, 0,0); // BRL- Jump to "else"
                    b.FromHere();
                    CODE.BitnessAnything();
                }
            }
            into.ToHere();
            into.Proceed();
            AddBranch(b, indent);
        }
        
        void CHARNAME_LOOP_BIG_BLOCK()
        {
            // X talteen tämän touhun ajaksi:
            CODE.Set16bit_X();
            CODE.Set16bit_M();
            CODE.EmitCode(0xDA); // PHX
             // Huom. Tässä välissä eivät muuttujaviittaukset toimi.
             SNEScode::RelativeBranch branchNonEpoch = CODE.PrepareRelativeBranch();
             SNEScode::RelativeBranch branchEpoch    = CODE.PrepareRelativeBranch();
             SNEScode::RelativeBranch branchNonMember= CODE.PrepareRelativeBranch();
             SNEScode::RelativeBranch branchMemberSkip=CODE.PrepareRelativeBranch();
             CODE.EmitCode(0xA9,0x00,0x00);  // lda $0000
             CODE.EmitCode(0xA8);            // tay
             CODE.Set8bit_M();
             CODE.EmitCode(0xA7, 0x31); // Id of the name
             CODE.EmitCode(0xC9, 0x20); // CMP A,$20 - If it's Epoch
             CODE.EmitCode(0xD0, 0);    // BNE - jump over if not
             branchNonEpoch.FromHere();
             // Yes, Epoch.
             CODE.Set16bit_M();
             CODE.EmitCode(0xA9, 0x4D, 0x2C);    // Load Epoch address $2C4D
             CODE.EmitCode(0x80, 0);             // BRA - jump to character name handling
             branchEpoch.FromHere();
             CODE.BitnessAnything();

             branchNonEpoch.ToHere();
             // No Epoch
             CODE.Set8bit_M();
             CODE.EmitCode(0xC9, 0x1E); // CMP A,$1B - if it's < [member1]
             CODE.EmitCode(0x90, 0);    // BCS - jump over if is <
             branchNonMember.FromHere();
             // Yes, it's [member1](1B) or [member2](1C) or [member3](1D).
             CODE.Set8bit_M();
             CODE.EmitCode(0x38,0xE9,0x1B);      // vähennetään $1B (sec; sbc A,$1B)
             CODE.EmitCode(0xAA);                // TAX
             CODE.EmitCode(0xBF,0x80,0x29,0x7E); // haetaan memberin numero
             CODE.EmitCode(0x80, 0);    // BRA - skip to mul2
             branchMemberSkip.FromHere();
             CODE.BitnessAnything();
             
             branchNonMember.ToHere();
             // No member
             CODE.Set8bit_M();
             CODE.EmitCode(0x38, 0xE9, 0x13); // vähennetään $13 (sec; sbc A,$13)
             branchMemberSkip.ToHere();
             CODE.Set8bit_M();
             CODE.EmitCode(0x0A, 0xAA);       // ASL A; TAX
             CODE.Set16bit_M();
             CODE.EmitCode(0xBF,0xD8,0x5F,0xC2); // haetaan pointteri nimeen
             branchEpoch.ToHere();
             CODE.Set16bit_M();
             CODE.EmitCode(0x8D,0x37,0x02);      // tallennetaan offset ($0237)
             CODE.Set8bit_M();
             CODE.EmitCode(0xA9,0x7E);           // tallennetaan segment ($0239)
             CODE.EmitCode(0x8D,0x39,0x02);
             branchEpoch.Proceed();
             branchNonEpoch.Proceed();
             branchMemberSkip.Proceed();
             branchNonMember.Proceed();
            CODE.Set16bit_X();
            CODE.Set16bit_M();
            CODE.EmitCode(0xFA); // PLX
            CODE.EmitCode(0x9B); // TXY
            
            // Hakee merkin hahmon nimestä
            CODE.Set16bit_M();
            CODE.EmitCode(0xB7,0x37);              //LDA [long[$00:D+$37]+Y]
            CODE.EmitCode(0x29, 0xFF, 0x00);       //AND A, $00FF

            Invalidate_A();
            AHstate.Const.Set(0);
            AHstate.Var.Invalidate();
        }
        void OUTBYTE_BIG_CODE()
        {
            CODE.Set16bit_X();
            CODE.Set16bit_M();
            
            CODE.EmitCode(0xDA);                   //PHX
            
             // Tässä välissä eivät muuttujaviittaukset toimi.
             CODE.EmitCode(0x85, 0x35);            //STA [$00:D+$35]
             SNEScode::FarToNearCall call = CODE.PrepareFarToNearCall();
             
             // call back the routine
             call.Proceed(0xC25DC4);
             CODE.BitnessUnknown();

            CODE.Set16bit_X();
            CODE.EmitCode(0xFA); // PLX

            Invalidate_A();
        }
        
        void START_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            loopbegin = new Branch(CODE.PrepareRelativeLongBranch());
            loopend   = new Branch(CODE.PrepareRelativeLongBranch());
            
            // Alkuarvo loopille
            CODE.Set16bit_M();
            CODE.EmitCode(0xA9,0x00,0x00);      // LDA $0000
            CODE.EmitCode(0xAA);                // TAX
            
            loopbegin->ToHere();
            
            cursub->CallSub(LoopHelperName);
            
            CODE.EmitCode(0xD0, 3);    // bne - jatketaan looppia, jos nonzero
            CODE.EmitCode(0x82, 0,0);  // brl - jump pois loopista.
            loopend->FromHere();
            CODE.BitnessAnything();

            Invalidate_A();

            STORE_VAR(MagicVarName);// save in "c".
            // mark read, because it indeed has been
            // read (in the loop end condition).
            vars[MagicVarName].read = true;
        }
        void END_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            CODE.EmitCode(0xE8);      // inx
            CODE.EmitCode(0x82, 0,0); // brl - jump back to loop
            loopbegin->FromHere();
            CODE.BitnessAnything();
            
            // loop end is here.        
            loopend->ToHere();

            Invalidate_A();
            
            loopbegin->Proceed();
            loopend->Proceed();
            
            delete loopbegin; loopbegin = NULL;
            delete loopend;   loopend   = NULL;
        }
        void OUT_CHARACTER()
        {
            CheckCodeStart();
            // outputs character in A
            
            cursub->CallSub(OutcHelperName);

            Invalidate_A();
        }
        #undef CODE
        
        const FunctionList &GetFunctions() const { return functions; }
    };
}

const FunctionList Compile(FILE *fp)
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
        
        if(words[0][0] == '#') continue;
        
        Asm.BRANCH_LEVEL(indent);
        
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
            if(words.size() > 1) Asm.LOAD_VAR(words[1]);
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
            Asm.STORE_VAR(words[2]);
        }
        else if(firstword == "IF")
        {
            if(words.size() > 2) Asm.LOAD_VAR(words[2]);
            Asm.CALL_FUNC(words[1]);
            Asm.COMPARE_BOOL(indent);
        }
        else if(firstword == "CALL")
        {
            if(words.size() > 2) Asm.LOAD_VAR(words[2]);
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
                Asm.LOAD_VAR(words[2]);
                Asm.COMPARE_EQUAL(words[1], indent);
            }
        }
        else if(firstword == ">")
        {
            Asm.LOAD_VAR(words[2]);
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
            Asm.LOAD_VAR(words[1]);
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

    Asm.START_FUNCTION(Asm.LoopHelperName);
    Asm.CHARNAME_LOOP_BIG_BLOCK();
    Asm.VOID_RETURN();
    Asm.END_FUNCTION();
    
    Asm.START_FUNCTION(Asm.OutcHelperName);
    Asm.OUTBYTE_BIG_CODE();
    Asm.VOID_RETURN();
    Asm.END_FUNCTION();
    
    return Asm.GetFunctions();
}

#include <string>
#include <set>
#include <map>

#include "snescode.hh"
#include "compiler.hh"
#include "ctcset.hh"

using namespace std;

#include "settings.hh"

#define OPTIMIZE_A

namespace
{
    class Assembler
    {
        SubRoutine *cursub;
        string CurSubName;
        
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
        
        typedef map<string, variable> vars_t;
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
        
#ifdef OPTIMIZE_A
        class aConstState
        {
            bool known;
            unsigned value;
        public:
            aConstState() : known(false),value(0) {}
            void Invalidate() { known=false; }
            bool Known() const { return known; }
            void Set(unsigned v) { known=true; value=v; }
            bool Is(unsigned v) const { return known && value==v; }
            void Inc() { if(known) ++value; }
            void Dec() { if(known) --value; }
        } aConstState;

        class aVarState
        {
            bool known;
            string var;
        public:
            aVarState() : known(false) {}
            void Invalidate() { known=false; }
            bool Known() const { return known; }
            void Set(const string &v) { known=true; var=v; }
            bool Is(const string &v) const { return known && var==v; }
        } aVarState;
        
        void Invalidate_A()
        {
            aVarState.Invalidate();
            aConstState.Invalidate();
        }
#endif
        
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
                CODE.AddCode(0x48); // PHA
                varcount -= 2;
            }
        #endif
            if(varcount >= 1) CODE.Set8bit_M();
            while(varcount >= 1)
            {
                CODE.AddCode(0x48); // PHA
                varcount -= 1;
            }
            endbranch = new Branch(CODE.PrepareRelativeLongBranch());
#ifdef OPTIMIZE_A
            aVarState.Set("c");
            aConstState.Invalidate();
#endif
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
                CODE.AddCode(0x7A); // PLY
                varcount -= 2;
            }
            if(varcount >= 1) CODE.Set8bit_X();
            while(varcount >= 1)
            {
                CODE.AddCode(0x7A); // PLY
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
                        CurSubName.c_str(), i->first.c_str(), warning);
                }
            }
            
            FINISH_BRANCHES();
            CODE.AddCode(0x6B);     // RTL
#if 0
            fprintf(stderr, "Function %s done:\n", CurSubName.c_str());
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
        unsigned char GetStackOffset(const string &varname) const
        {
            vars_t::const_iterator i = vars.find(varname);
            if(i == vars.end())
            {
                fprintf(stderr, "ERROR: In function '%s': Undefined variable '%s'\n",
                    CurSubName.c_str(), varname.c_str());
                return 0;
            }
            if(i->second.is_regvar)
            {
                fprintf(stderr, "ERROR: In function '%s': "
                                "'%s defined with REG, but isn't in registers\n",
                     CurSubName.c_str(), varname.c_str());
                return 0;
            }
            
            return i->second.stackpos;
        }
    public:
        Assembler() : cursub(NULL)
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
#ifdef OPTIMIZE_A
                    Invalidate_A();
#endif
                    a->second.ToHere();
                    a->first = 999; // prevent being reassigned
                }
            }
        }
        void START_FUNCTION(const string &name)
        {
            CurSubName = name;
            started = false;
            vars.clear();

            cursub = new SubRoutine;
            
#ifdef OPTIMIZE_A
            Invalidate_A();
#endif
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
        void DECLARE_VAR(const string &name)
        {
            if(vars.find(name) == vars.end())
            {
                // pino-osoitteet pienenev�t pushatessa.
                //    s = 6E5
                //   pha - this goes to 6E5
                //   pha - this goes to 6E4
                //   pha - this goes to 6E3
                // t�m�n j�lkeen s = 6E2.
                // Thus, [s+1] refers to var1, [s+2] to var2, and so on.
                
                unsigned stackpos = vars.size() + 1;
                
                vars[name].is_regvar = false;
                vars[name].stackpos = stackpos;
            }
        }
        void DECLARE_REGVAR(const string &name)
        {
            vars[name].is_regvar = true;
        }
        void VOID_RETURN()
        {
            CheckCodeStart();

            CODE.AddCode(0x82, 0,0); // BRL- Jump to end
            endbranch->FromHere();
        }
        void BOOLEAN_RETURN(bool value)
        {
            CheckCodeStart();
            // emit flag, then return
            if(value)
                CODE.AddCode(0x18); // CLC - carry nonset = true
            else
                CODE.AddCode(0x38); // SEC - carry set = false
            VOID_RETURN();
        }
        void LOAD_VAR(const string &name)
        {
            CheckCodeStart();
            // load var to A
            
            if(vars.find(name) != vars.end())
            {
                vars[name].read = true;
#ifdef OPTIMIZE_A
                if(aVarState.Is(name)) return;

                aVarState.Set(name);
                aConstState.Invalidate();
#endif
                unsigned stackpos = GetStackOffset(name);
                vars[name].loaded = true;
                if(!vars[name].written)
                {
                    fprintf(stderr,
                        "  Warning: In function '%s', variable '%s' was read before written.\n",
                            CurSubName.c_str(), name.c_str());
                }
                CODE.AddCode(0xA3, stackpos); // LDA [00:s+n]
                
                return;
            }
            
            unsigned val = 0;
            
            if(name[0] == '\'')
                val = (unsigned char) getchronochar((unsigned char)name[1]);
            else
                val = strtol(name.c_str(), NULL, 10);
            
            if(val >= 256)
            {
                fprintf(stderr,
                    "  Warning: In function '%s', numeric constant %u too large (>255)\n",
                    CurSubName.c_str(), val);
            }
            
#ifdef OPTIMIZE_A
            if(aConstState.Is(val)) return;
            
            aConstState.Set(val);
            aVarState.Invalidate();
#endif
            
            if(val < 256)
            {
                CODE.Set8bit_M();
                CODE.AddCode(0xA9, val); // LDA A, imm8
            }
            else
            {
                CODE.Set16bit_M();
                CODE.AddCode(0xA9, val&255, val>>8); // LDA A, imm16
            }
        }
        void STORE_VAR(const string &name)
        {
#ifdef OPTIMIZE_A
            if(aVarState.Is(name)) return;
#endif

            if(vars.find(name) == vars.end()
            || !vars.find(name)->second.is_regvar)
            {
                CheckCodeStart();
                // store A to var
                CODE.Set8bit_M();
                unsigned stackpos = GetStackOffset(name);
                CODE.AddCode(0x83, stackpos); // STA [00:s+n]
            }
            vars[name].written = true;
            
#ifdef OPTIMIZE_A
            aVarState.Set(name);
#endif
        }
        void INC_VAR(const string &name)
        {
            CheckCodeStart();
            // inc var
            LOAD_VAR(name);
            CODE.Set8bit_M();
            CODE.AddCode(0x1A); // INC A
#ifdef OPTIMIZE_A
            aVarState.Invalidate();
            aConstState.Inc();
#endif
            STORE_VAR(name);
        }
        void DEC_VAR(const string &name)
        {
            CheckCodeStart();
            // dec var
            LOAD_VAR(name);
            CODE.Set8bit_M();
            CODE.AddCode(0x3A); // DEC A
#ifdef OPTIMIZE_A
            aVarState.Invalidate();
            aConstState.Dec();
#endif
            STORE_VAR(name);
        }
        void CALL_FUNC(const string &name)
        {
            CheckCodeStart();
            
            CODE.Set16bit_X();
            CODE.AddCode(0xDA); // PHX
            
            // leave A unmodified and issue call to function
            CODE.AddCode(0x22, 0,0,0);
            cursub->requires[name].insert(CODE.size() - 3);
            CODE.BitnessUnknown();
            
            CODE.Set16bit_X();
            CODE.AddCode(0xFA); // PLX
#ifdef OPTIMIZE_A
            Invalidate_A();
#endif
        }
        void COMPARE_BOOL(unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if boolean set
            
            Branch b = CODE.PrepareRelativeLongBranch();
            CODE.AddCode(0x90, 3);   // BCC- Jump to if true
            CODE.AddCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            AddBranch(b, indent);
        }
        void COMPARE_EQUAL(const string &name, unsigned indent)
        {
            CheckCodeStart();
            
            unsigned stackpos = GetStackOffset(name);
            vars[name].read = vars[name].loaded = true;
            
            // process subblock if A is equal to given var
            CODE.AddCode(0xC3, stackpos); // CMP [00:s+n]

            Branch b = CODE.PrepareRelativeLongBranch();
            CODE.AddCode(0xF0, 3);   // BEQ- Jump to if eq
            CODE.AddCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            AddBranch(b, indent);
        }
        void COMPARE_ZERO(const string &name, unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if var is zero
            LOAD_VAR(name);

            Branch b = CODE.PrepareRelativeLongBranch();
            CODE.AddCode(0xF0, 3);   // BEQ- Jump to if zero
            CODE.AddCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            AddBranch(b, indent);
        }
        void COMPARE_GREATER(const string &name, unsigned indent)
        {
            CheckCodeStart();

            unsigned stackpos = GetStackOffset(name);
            vars[name].read = vars[name].loaded = true;

            // process subblock if A is greater than given var
            CODE.AddCode(0xC3, stackpos); // CMP [00:s+n]
            
            Branch b = CODE.PrepareRelativeLongBranch();
            CODE.AddCode(0xB0, 3);   // BCS- Jump to if greater or equal
            CODE.AddCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            CODE.AddCode(0xD0, 3);   // BCS- Jump to n-eq too (leaving only "greater")
            CODE.AddCode(0x82, 0,0); // BRL- Jump to "else"
            b.FromHere();
            
            AddBranch(b, indent);
        }
        void SELECT_CASE(const string &cset, unsigned indent)
        {
            if(!cset.size()) return;
            
            CheckCodeStart();

            // process subblock if A is in any of given chars
            Branch b = CODE.PrepareRelativeLongBranch();
            
            SNEScode::RelativeBranch into = CODE.PrepareRelativeBranch();
            for(unsigned a=0; a<cset.size(); ++a)
            {
                unsigned char c = getchronochar((unsigned char)cset[a]);
                CODE.Set8bit_M();
                CODE.AddCode(0xC9, c);   // CMP A, imm8            
                CODE.AddCode(0xF0, 0);   // BEQ - Jump to if equal
                into.FromHere();
                
                if(a+1 == cset.size())
                {
                    // If this was the last comparison, do the "else" here.
                    CODE.AddCode(0x82, 0,0); // BRL- Jump to "else"
                    b.FromHere();
                }
            }
            into.ToHere();
            into.Proceed();
            AddBranch(b, indent);
        }
        
        void CHARNAME_LOOP_BIG_BLOCK()
        {
            // X talteen t�m�n touhun ajaksi:
            CODE.Set16bit_X();
            CODE.Set16bit_M();
            CODE.AddCode(0xDA); // PHX
             // Huom. T�ss� v�liss� eiv�t muuttujaviittaukset toimi.
             SNEScode::RelativeBranch branchNonEpoch = CODE.PrepareRelativeBranch();
             SNEScode::RelativeBranch branchEpoch    = CODE.PrepareRelativeBranch();
             SNEScode::RelativeBranch branchNonMember= CODE.PrepareRelativeBranch();
             SNEScode::RelativeBranch branchMemberSkip=CODE.PrepareRelativeBranch();
             CODE.AddCode(0xA9,0x00,0x00);  // lda $0000
             CODE.AddCode(0xA8);            // tay
             CODE.Set8bit_M();
             CODE.AddCode(0xA7, 0x31); // Id of the name
             CODE.AddCode(0xC9, 0x20); // CMP A,$20 - If it's Epoch
             CODE.AddCode(0xD0, 0);    // BNE - jump over if not
             branchNonEpoch.FromHere();
             // Yes, Epoch.
             CODE.Set16bit_M();
             CODE.AddCode(0xA9, 0x4D, 0x2C);    // Load Epoch address $2C4D
             CODE.AddCode(0x80, 0);             // BRA - jump to character name handling
             branchEpoch.FromHere();

             branchNonEpoch.ToHere();
             // No Epoch
             CODE.Set8bit_M();
             CODE.AddCode(0xC9, 0x1E); // CMP A,$1B - if it's < [member1]
             CODE.AddCode(0x90, 0);    // BCS - jump over if is <
             branchNonMember.FromHere();
             // Yes, it's [member1](1B) or [member2](1C) or [member3](1D).
             CODE.Set8bit_M();
             CODE.AddCode(0x38,0xE9,0x1B);      // v�hennet��n $1B (sec; sbc A,$1B)
             CODE.AddCode(0xAA);                // TAX
             CODE.AddCode(0xBF,0x80,0x29,0x7E); // haetaan memberin numero
             CODE.AddCode(0x80, 0);    // BRA - skip to mul2
             branchMemberSkip.FromHere();
             
             branchNonMember.ToHere();
             // No member
             CODE.Set8bit_M();
             CODE.AddCode(0x38, 0xE9, 0x13); // v�hennet��n $13 (sec; sbc A,$13)
             branchMemberSkip.ToHere();
             CODE.Set8bit_M();
             CODE.AddCode(0x0A, 0xAA);       // ASL A; TAX
             CODE.Set16bit_M();
             CODE.AddCode(0xBF,0xD8,0x5F,0xC2); // haetaan pointteri nimeen
             branchEpoch.ToHere();
             CODE.Set16bit_M();
             CODE.AddCode(0x8D,0x37,0x02);      // tallennetaan offset ($0237)
             CODE.Set8bit_M();
             CODE.AddCode(0xA9,0x7E);           // tallennetaan segment ($0239)
             CODE.AddCode(0x8D,0x39,0x02);
             branchEpoch.Proceed();
             branchNonEpoch.Proceed();
             branchMemberSkip.Proceed();
             branchNonMember.Proceed();
            CODE.Set16bit_X();
            CODE.Set16bit_M();
            CODE.AddCode(0xFA); // PLX
            CODE.AddCode(0x9B); // TXY
            
            // Hakee merkin hahmon nimest�
            CODE.Set8bit_M();
            CODE.AddCode(0xB7,0x37);  // lda [long[$00:D+$37]+y]
#ifdef OPTIMIZE_A
            Invalidate_A();
#endif
        }
        void OUTBYTE_BIG_CODE()
        {
            CODE.Set16bit_X();
            CODE.Set8bit_M();
            CODE.AddCode(0xDA); // PHX
            
             // T�ss� v�liss� eiv�t muuttujaviittaukset toimi.
             CODE.AddCode(0x85, 0x35); //sta [$00:D+$35]
             SNEScode::FarToNearCall call = CODE.PrepareFarToNearCall();
             
             // call back the routine
             call.Proceed(0xC25DC4);
             CODE.BitnessUnknown();

            CODE.Set16bit_X();
            CODE.AddCode(0xFA); // PLX
#ifdef OPTIMIZE_A
            Invalidate_A();
#endif
        }
        
        void START_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            loopbegin = new Branch(CODE.PrepareRelativeLongBranch());
            loopend   = new Branch(CODE.PrepareRelativeLongBranch());
            
            // Alkuarvo loopille
            CODE.Set16bit_M();
            CODE.AddCode(0xA9,0x00,0x00);      // LDA $0000
            CODE.AddCode(0xAA);                // TAX
            
            loopbegin->ToHere();
            CODE.AddCode(0x22, 0,0,0);
            cursub->requires[LoopHelperName].insert(CODE.size() - 3);

            CODE.BitnessUnknown();
            
            CODE.AddCode(0xD0, 3);    // bne - jatketaan looppia, jos nonzero
            CODE.AddCode(0x82, 0,0);  // brl - jump pois loopista.
            loopend->FromHere();
#ifdef OPTIMIZE_A
            Invalidate_A();
#endif

            STORE_VAR("c");        // save in "c".
            vars["c"].read = true; // mark read, because it indeed has been
                                   // read (in the loop end condition).
        }
        void END_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            CODE.AddCode(0xE8);      // inx
            CODE.AddCode(0x82, 0,0); // brl - jump back to loop
            loopbegin->FromHere();
            
            // loop end is here.        
            loopend->ToHere();
#ifdef OPTIMIZE_A
            Invalidate_A();
#endif
            
            loopbegin->Proceed();
            loopend->Proceed();
            
            delete loopbegin; loopbegin = NULL;
            delete loopend;   loopend   = NULL;
        }
        void OUT_CHARACTER()
        {
            CheckCodeStart();
            // outputs character in A
            
            CODE.AddCode(0x22, 0,0,0);
            cursub->requires[OutcHelperName].insert(CODE.size() - 3);

            CODE.BitnessUnknown();
#ifdef OPTIMIZE_A
            Invalidate_A();
#endif
        }
        #undef CODE
        
        const FunctionList &GetFunctions() const { return functions; }
    };
}

void FunctionList::Define(const string &name, const SubRoutine &sub)
{
    functions[name] = make_pair(sub, false);
}

void FunctionList::RequireFunction(const string &name)
{
    functions_t::iterator i = functions.find(name);
    if(i == functions.end())
    {
        fprintf(stderr, "Error: Function '%s' not defined!\n", name.c_str());
        return;
    }
    bool &required         = i->second.second;
    const SubRoutine &func = i->second.first;
    
    if(required) return;
#if 0
    fprintf(stderr, "Requiring function '%s' (%u bytes)...\n",
        name.c_str(), func.code.size());
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

const FunctionList Compile(FILE *fp)
{
    Assembler Asm;
    
    char Buf[256];
    while((fgets(Buf, sizeof Buf, fp)))
    {
        unsigned indent=0;
        vector<string> words;
        
        if(1) // Initialize indent, words
        {
            char *s;
            for(s=Buf; *s; ++s){if(*s=='\n'||*s=='\r'){*s=0;break;}}
            for(s=Buf; *s==' '; ++s) ++indent;

            string rest = s;
            for(;;)
            {
                unsigned spacepos = rest.find(' ');
                if(spacepos == rest.npos)break;
                words.push_back(rest.substr(0, spacepos));
                if(spacepos+1 >= rest.size()) { rest = ""; break; }
                rest = rest.substr(spacepos+1);
            }
            if(rest.size()) { words.push_back(rest); rest = ""; }
        }
        
        if(!words.size()) continue;
        
        if(words[0][0] == '#') continue;
        
        Asm.BRANCH_LEVEL(indent);
        
        if(words[0] == "TRUE")
        {
            Asm.BOOLEAN_RETURN(true);
        }
        else if(words[0] == "FALSE")
        {
            Asm.BOOLEAN_RETURN(false);
        }
        else if(words[0] == "RETURN")
        {
            if(words.size() > 1) Asm.LOAD_VAR(words[1]);
            Asm.VOID_RETURN();
        }
        else if(words[0] == "VAR")
        {
            for(unsigned a=1; a<words.size(); ++a)
            {
                Asm.DECLARE_VAR(words[a]);
            }
        }
        else if(words[0] == "REG")
        {
            for(unsigned a=1; a<words.size(); ++a)
            {
                Asm.DECLARE_REGVAR(words[a]);
            }
        }
        else if(words[0] == "CALL_GET")
        {
            Asm.CALL_FUNC(words[1]);
            Asm.STORE_VAR(words[2]);
        }
        else if(words[0] == "IF")
        {
            if(words.size() > 2) Asm.LOAD_VAR(words[2]);
            Asm.CALL_FUNC(words[1]);
            Asm.COMPARE_BOOL(indent);
        }
        else if(words[0] == "CALL")
        {
            if(words.size() > 2) Asm.LOAD_VAR(words[2]);
            Asm.CALL_FUNC(words[1]);
        }
        else if(words[0] == "INC")
        {
            Asm.INC_VAR(words[1]);
        }
        else if(words[0] == "DEC")
        {
            Asm.DEC_VAR(words[1]);
        }
        else if(words[0] == "LET")
        {
            Asm.LOAD_VAR(words[2]);
            Asm.STORE_VAR(words[1]);
        }
        else if(words[0] == "=")
        {
            if(words[2] == "0")
                Asm.COMPARE_ZERO(words[1], indent);
            else
            {
                Asm.LOAD_VAR(words[2]);
                Asm.COMPARE_EQUAL(words[1], indent);
            }
        }
        else if(words[0] == ">")
        {
            Asm.LOAD_VAR(words[2]);
            Asm.COMPARE_GREATER(words[1], indent);
        }
        else if(words[0] == "?")
        {
            Asm.LOAD_VAR(words[1]);
            Asm.SELECT_CASE(words[2], indent);
        }
        else if(words[0] == "{")
        {
            Asm.START_CHARNAME_LOOP();
        }
        else if(words[0] == "}")
        {
            Asm.END_CHARNAME_LOOP();
        }
        else if(words[0] == "OUT")
        {
            Asm.LOAD_VAR(words[1]);
            Asm.OUT_CHARACTER();
        }
        else if(words[0] == "FUNCTION")
        {
            Asm.END_FUNCTION();
            Asm.START_FUNCTION(words[1]);
        }
        else
            fprintf(stderr, "  ERROR: What's this? '%s'\n", words[0].c_str());
    }
    Asm.END_FUNCTION();

    Asm.START_FUNCTION(LoopHelperName);
    Asm.CHARNAME_LOOP_BIG_BLOCK();
    Asm.VOID_RETURN();
    Asm.END_FUNCTION();
    
    Asm.START_FUNCTION(OutcHelperName);
    Asm.OUTBYTE_BIG_CODE();
    Asm.VOID_RETURN();
    Asm.END_FUNCTION();
    
    return Asm.GetFunctions();
}

#include <string>
#include <set>
#include <map>

#include "snescode.hh"
#include "compiler.hh"
#include "ctcset.hh"

using namespace std;

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

class Assembler
{
    SubRoutine *cursub;
    string CurSubName;
    
    // varname -> stack index
    typedef map<string, unsigned> vars_t;
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
    
    void AddBranch(const Branch &b, unsigned ind)
    {
        openbranches.push_back(make_pair(ind, b));
    }
    
    void FRAME_BEGIN()
    {
        unsigned varcount = vars.size();
        if(varcount >= 2) CODE.Set16bit_M();
        //CODE.AddCode(0x08);     // PHP
        while(varcount >= 2)
        {
            CODE.AddCode(0x48); // PHA
            varcount -= 2;
        }
        if(varcount >= 1) CODE.Set8bit_M();
        while(varcount >= 1)
        {
            CODE.AddCode(0x48); // PHA
            varcount -= 1;
        }
        endbranch = new Branch(CODE.PrepareRelativeLongBranch());
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
        //CODE.AddCode(0x28);     // PLP
        
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
        // pino-osoitteet pienenevät pushatessa.
        //    s = 6E5
        //   pha - this goes to 6E5
        //   pha - this goes to 6E4
        //   pha - this goes to 6E3
        // tämän jälkeen s = 6E2.
        
        vars_t::const_iterator i = vars.find(varname);
        if(i == vars.end()) return 0;
        
        // Thus, [s+1] refers to var1, [s+2] to var2, and so on.
        return i->second+1;
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
        DECLARE_VAR("c");
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
            unsigned varnum = vars.size();
             vars[name] = varnum;
        }
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
            CODE.AddCode(0xA3, GetStackOffset(name)); // LDA [00:s+n]
        }
        else if(name[0] == '\'')
        {
            CODE.Set8bit_M();
            unsigned char c = getchronochar((unsigned char)name[1]);
            CODE.AddCode(0xA9, c); // LDA A, imm8
        }
        else
        {
            unsigned val = strtol(name.c_str(), NULL, 10);
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
    }
    void STORE_VAR(const string &name)
    {
        CheckCodeStart();
        // store A to var
        CODE.Set8bit_M();
        CODE.AddCode(0x83, GetStackOffset(name)); // STA [00:s+n]
    }
    void INC_VAR(const string &name)
    {
        CheckCodeStart();
        // inc var
        LOAD_VAR(name);
        CODE.AddCode(0x1A); // INC A
        STORE_VAR(name);
    }
    void DEC_VAR(const string &name)
    {
        CheckCodeStart();
        // dec var
        LOAD_VAR(name);
        CODE.AddCode(0x3A); // DEC A
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
        
        // process subblock if A is equal to given var
        CODE.AddCode(0xC3, GetStackOffset(name)); // CMP [00:s+n]

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

        // process subblock if A is greater than given var
        CODE.AddCode(0xC3, GetStackOffset(name)); // CMP [00:s+n]
        
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
            // FIXME: Tämä ei toimi oikein!
        }
        into.ToHere();
        into.Proceed();
        AddBranch(b, indent);
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
        
        /* FIXME: Not really intelligent to repeat this in all! */
        
        loopbegin->ToHere();
        
        // TÄHÄN LABEL

        // X talteen tämän touhun ajaksi:
        CODE.Set16bit_X();
        CODE.Set16bit_M();
        CODE.AddCode(0xDA); // PHX
         // Huom. Tässä välissä eivät muuttujaviittaukset toimi.
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
         CODE.AddCode(0x38,0xE9,0x1B);      // vähennetään $1B (sec; sbc A,$1B)
         CODE.AddCode(0xAA);                // TAX
         CODE.AddCode(0xBF,0x80,0x29,0x7E); // haetaan memberin numero
         CODE.AddCode(0x80, 0);    // BRA - skip to mul2
         branchMemberSkip.FromHere();
         
         branchNonMember.ToHere();
         // No member
         CODE.Set8bit_M();
         CODE.AddCode(0x38, 0xE9, 0x13); // vähennetään $13 (sec; sbc A,$13)
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
        
        // Hakee merkin hahmon nimestä
        CODE.Set8bit_M();
        CODE.AddCode(0xB7,0x37);  // lda [long[$00:D+$37]+y]
        CODE.AddCode(0xD0, 3);    // bne - jatketaan looppia, jos nonzero
        CODE.AddCode(0x82, 0,0);  // brl - jump pois loopista.
        loopend->FromHere();
        
        STORE_VAR("c"); // talteen c:hen.
    }
    void END_CHARNAME_LOOP()
    {
        CheckCodeStart();
        
        CODE.AddCode(0xE8);      // inx
        CODE.AddCode(0x82, 0,0); // brl - jump back to loop
        loopbegin->FromHere();
        
        // loop end is here.        
        loopend->ToHere();
        
        loopbegin->Proceed();
        loopend->Proceed();
        
        delete loopbegin; loopbegin = NULL;
        delete loopend;   loopend   = NULL;
    }
    void OUT_CHARACTER()
    {
        CheckCodeStart();
        // outputs character in A
        
        CODE.Set16bit_X();
        CODE.Set8bit_M();
        CODE.AddCode(0xDA); // PHX
        
         // Tässä välissä eivät muuttujaviittaukset toimi.
         CODE.AddCode(0x85, 0x35); //sta [$00:D+$35]
         SNEScode::FarToNearCall call = CODE.PrepareFarToNearCall();
         
         // call back the routine
         CODE.Set16bit_M();
         CODE.AddCode(0xA5, 0x35); //lda [$00:D+$35]
         
         call.Proceed(0xC25DC8);
         CODE.BitnessUnknown();

        CODE.Set16bit_X();
        CODE.AddCode(0xFA); // PLX
    }
    #undef CODE
    
    const FunctionList &GetFunctions() const { return functions; }
};

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
        else if(words[0] == "RET")
        {
            Asm.VOID_RETURN();
        }
        else if(words[0] == "RETURN")
        {
            Asm.LOAD_VAR(words[1]);
            Asm.VOID_RETURN();
        }
        else if(words[0] == "VAR")
        {
            for(unsigned a=1; a<words.size(); ++a)
                Asm.DECLARE_VAR(words[a]);
        }
        else if(words[0] == "ci")
        {
            Asm.CALL_FUNC(words[1]);
            Asm.STORE_VAR(words[2]);
        }
        else if(words[0] == "cb")
        {
            Asm.CALL_FUNC(words[1]);
            Asm.COMPARE_BOOL(indent);
        }
        else if(words[0] == "cq")
        {
            Asm.LOAD_VAR(words[2]);
            Asm.CALL_FUNC(words[1]);
            Asm.COMPARE_BOOL(indent);
        }
        else if(words[0] == "cv")
        {
            Asm.LOAD_VAR(words[2]);
            Asm.CALL_FUNC(words[1]);
        }
        else if(words[0] == "cf")
        {
            Asm.CALL_FUNC(words[1]);
        }
        else if(words[0] == "+")
        {
            Asm.INC_VAR(words[1]);
        }
        else if(words[0] == "-")
        {
            Asm.DEC_VAR(words[1]);
        }
        else if(words[0] == "s")
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
            Asm.LOAD_VAR("c");
            Asm.SELECT_CASE(words[1], indent);
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
        else if(words[0] == ":B"
             || words[0] == ":Q"
             || words[0] == ":I")
        {
            Asm.END_FUNCTION();
            Asm.START_FUNCTION(words[1]);
        }
    }
    Asm.END_FUNCTION();
    
    return Asm.GetFunctions();
}

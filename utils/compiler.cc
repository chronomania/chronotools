#include <cstdio>
#include <set>
#include <map>
#include <list>

#include "wstring.hh"
#include "config.hh"
#include "ctcset.hh"
#include "tristate"

using namespace std;

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

        // code begun?
        bool started;
        
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
        
        void AddBranch(const char* name, unsigned ind)
        {
            openbranches.push_back(make_pair(ind, name));
        }
        
        void FRAME_BEGIN()
        {
            unsigned varcount = vars.size();
            /* Note: All vars will be initialized with A's current value */
        #if 0
            if(varcount >= 2) Emit_M(16);
            while(varcount >= 2)
            {
                Emit("pha");
                varcount -= 2;
            }
        #endif
            if(varcount >= 1) Emit_M(8);
            while(varcount >= 1)
            {
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
            
            EmitLabel("End");
            
            unsigned varcount = vars.size();
            if(varcount >= 2) Emit_X(16);
            while(varcount >= 2)
            {
                Emit("ply");
                varcount -= 2;
            }
            if(varcount >= 1) Emit_X(8);
            while(varcount >= 1)
            {
                Emit("ply");
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
            Emit("rts");
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

        enum { StateUnknown, StateSet, StateUnset, StateAnything }
            StateX, StateM,
            WantedX, WantedM;
        bool newline;
        unsigned indent;

        void Flushbits()
        {
            unsigned rep = 0;
            unsigned sep = 0;
            if(WantedX != StateAnything && WantedX != StateX)
            {
                if(WantedX == StateSet) sep |= 0x10; else rep |= 0x10;
                StateX = WantedX;
            }
            if(WantedM != StateAnything && WantedM != StateM)
            {
                if(WantedM == StateSet) sep |= 0x20; else rep |= 0x20;
                StateM = WantedM;
            }
            if(sep & 0x10) { Emit(".xs"); EmitNoNewline(); }
            if(sep & 0x20) { Emit(".as"); EmitNoNewline(); }
            if(rep & 0x10) { Emit(".xl"); EmitNoNewline(); }
            if(rep & 0x20) { Emit(".al"); EmitNoNewline(); }
            if(sep)EmitImmed("sep", sep);
            if(rep)EmitImmed("rep", rep);
        }
        
        void Emit_M(unsigned bitness)
        {
            WantedM = bitness==16 ? StateUnset : StateSet;
        }
        void Emit_X(unsigned bitness)
        {
            WantedX = bitness==16 ? StateUnset : StateSet;
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
            
            if(!strcmp(code, ".)")) if(indent > 0) --indent;
            
            if(newline) fputc('\n', OutFile);
            else fprintf(OutFile, " : ");
            
            if(*code != '+' && newline)
            {
                fprintf(OutFile, "\t%*s", indent, "");
            }
            fprintf(OutFile, "%s", code);
            newline = true;
            
            if(*code == 'c') newline = false;
            if(strchr(code, ';')) newline = true;

            if(!strcmp(code, ".(")) ++indent;
        }

    public:
        void EmitNoNewline()
        {
            newline = false;
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
            newline = true;
            indent = 0;
        }
        
        void BRANCH_LEVEL(unsigned indent)
        {
            for(branchlist_t::iterator
                a=openbranches.begin();
                a!=openbranches.end();
                ++a)
            {
                if(a->first != 999 && indent <= a->first)
                {
                    Invalidate_A();

                    EmitLabel(a->second);
                    Emit(".)");
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
        }
        void END_FUNCTION()
        {
            CheckCodeStart();
            FRAME_END();
            if(!CurSubName.empty())
            {
            	Emit(".)");
            	Emit("#endif");
            	Emit("\n");
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
            
            Emit("brl End");
        }
        void BOOLEAN_RETURN(bool value)
        {
            CheckCodeStart();
            // emit flag, then return
            if(value)
                Emit("clc"); // CLC - carry nonset = true
            else
                Emit("sec"); // SEC - carry set = false
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
                        Emit_M(16);
                        Emit("lda #0");
                        ALstate.Const.Set(0);
                        ALstate.Var.Invalidate();
                        AHstate.Const.Set(0);
                        AHstate.Var.Invalidate();
                    }
                    else
                    {
                        Emit("xba");
                        AHstate = ALstate;
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
            
            if(!AHstate.Const.Is(hi))
            {
                Emit_M(16);
                EmitImmed("lda", val);
                ALstate.Const.Set(lo);
                ALstate.Var.Invalidate();
                AHstate.Const.Set(hi);
                AHstate.Var.Invalidate();
                return;
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
            
            Emit_X(16);
            Emit("phx");
            
            // leave A unmodified and issue call to function
            EmitCall(name);
            
            Emit_X(16);
            Emit("plx");

            Invalidate_A();
        }
        void COMPARE_BOOL(unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if boolean set
            
            Emit(".(");
            Emit("bcc +");
            Emit("brl Else");
            Emit("+");
            //EmitAnyBits();
            AddBranch("Else", indent);
        }
        void COMPARE_EQUAL(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();
            
            unsigned stackpos = GetStackOffset(name);
            vars[name].read = vars[name].loaded = true;
            
            Emit_M(8);
            
            // process subblock if A is equal to given var
            // Note: we're not checking ALstate here, would be mostly useless check
            EmitStack("cmp", stackpos);
            
            Emit(".(");
            Emit("beq +");
            Emit("brl Else");
            Emit("+");
            //EmitAnyBits(); 
            AddBranch("Else", indent);
        }
        void COMPARE_ZERO(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();
            
            // process subblock if var is zero
            LOAD_VAR(name);
            EmitNoNewline();

            // Note: we're not checking ALstate here, would be mostly useless check

            Emit(".(");
            Emit("beq +");
            Emit("brl Else");
            Emit("+");
            //EmitAnyBits();
            AddBranch("Else", indent);
        }
        void COMPARE_GREATER(const ucs4string &name, unsigned indent)
        {
            CheckCodeStart();

            unsigned stackpos = GetStackOffset(name);
            vars[name].read = vars[name].loaded = true;

            Emit_M(8);
            
            // process subblock if A is greater than given var
            EmitStack("cmp", stackpos);
            
            Emit(".(");
            Emit("bcs +");    // BCS- Jump to if greater or equal
            Emit("brl Else"); // BRL- Jump to "else"
            Emit("+");
            //EmitAnyBits();
            Emit("bne +");    // BNE- Jump to n-eq too (leaving only "greater")
            Emit("brl Else"); // BRL- Jump to "else"
            Emit("+");
            //EmitAnyBits();
            AddBranch("Else", indent);
        }
        void SELECT_CASE(const ucs4string &cset, unsigned indent)
        {
            if(cset.empty()) return;
            
            CheckCodeStart();

            // process subblock if A is in any of given chars
            
            Emit(".(");
            
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
            for(unsigned a=0; a<rangebegins.size(); ++a)
            {
                const bool last = (a+1) == rangebegins.size();
                const ctchar c1 = rangebegins[a];
                const ctchar c2 = rangeends[a];
                Emit_M(8);
                if(last)
                {
                    if(c1 == c2)
                    {
                        EmitImmed("cmp", c1); Emit("bne Else");
                    }
                    else
                    {
                        EmitImmed("cmp", c1);   Emit("bcc Else");
                        if(c2 < 0xFF)
                        {
                            EmitImmed("cmp", c2+1); Emit("bcs Else");
                        }
                    }
                }
                else
                {
                    if(c1 == c2)
                    {
                    	used_next = true;
                        EmitImmed("cmp", c1); Emit("beq +");
                    }
                    else
                    {
                        EmitImmed("cmp", c1);   Emit("bcc Else");
                        if(c2 >= 0xFF)
                        {
                        	used_next = true;
                            Emit("bra +");
                        }
                        else
                        {
                            EmitImmed("cmp", c2+1); Emit("bcc +");
                        }
                    }
                }
            }
            if(used_next) Emit("+");
            //EmitAnyBits();
            AddBranch("Else", indent);
        }
        void START_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            Emit(".(");
            Emit("; Init loop");
            Emit_M(16);
            Emit_X(16);
            Emit("ldx #0");
            EmitLabel("LoopBegin");
            EmitCall(LoopHelperName);
            Emit("; Continue loop if nonzero");
            Emit("bne +");
            Emit("brl LoopEnd");
            Emit("+");

            Invalidate_A();

            STORE_VAR(MagicVarName);// save in "c".
            // mark read, because it indeed has been
            // read (in the loop end condition).
            vars[MagicVarName].read = true;
        }
        void END_CHARNAME_LOOP()
        {
            CheckCodeStart();
            
            Emit("inx");
            Emit("brl LoopBegin");
            EmitLabel("LoopEnd");

            Invalidate_A();
            
            Emit(".)");
        }
        void OUT_CHARACTER()
        {
            CheckCodeStart();
            // outputs character in A
            
            EmitCall(OutcHelperName);

            Invalidate_A();
        }
        #undef CODE
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
            string s = "; ";
            s += WstrToAsc(Buf).c_str() + 1;
            fprintf(OutFile, "%s\n", s.c_str());
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
                Asm.LOAD_VAR(words[2]);
                Asm.EmitNoNewline();
                Asm.COMPARE_EQUAL(words[1], indent);
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
            Asm.LOAD_VAR(words[1]);
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

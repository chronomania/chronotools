#include <cstdio>
#include <cstdarg>

#include "settings.hh"
#include "symbols.hh"
#include "miscfun.hh"
#include "ctcset.hh"
#include "config.hh"
#include "space.hh"
#include "ctinsert.hh"
#include "conjugate.hh"

using namespace std;

void Conjugatemap::Load(const insertor &ins)
{
    const ConfParser::ElemVec& elems = GetConf("conjugator", "setup").Fields();
    for(unsigned a=0; a<elems.size(); a += 3)
    {
        form tmp;
        const ucs4string &func  = elems[a];
        const ucs4string &data  = elems[a+1];
        const ucs4string &width = elems[a+2];
        
        tmp.func   = func;
        tmp.used   = false;
        tmp.prefix = 0;
        
        tmp.maxwidth = 0;
        for(unsigned b=0; b<width.size(); ++b)
        {
            ctchar c = getchronochar(width[b], cset_12pix);
            tmp.maxwidth += 1 + ins.GetFont12width(c);
        }
        
        for(unsigned b=0; b<data.size(); ++b)
        {
            if(data[b] == ' ' || data[b] == '\n'
            || data[b] == '\r' || data[b] == '\t') continue;
            
            ctchar person = 0x13;
            switch(WcharToAsc(data[b]))
            {
                case 'c': person=0x13; break;
                case 'm': person=0x14; break;
                case 'l': person=0x15; break;
                case 'r': person=0x16; break;
                case 'f': person=0x17; break;
                case 'a': person=0x18; break;
                case 'u': person=0x19; break;
                case 'e': person=0x20; break;
                case '1': person=0x1B; break;
                case '2': person=0x1C; break;
                case '3': person=0x1D; break;
                default:
                    fprintf(stderr, "In configuration: Unknown person '%c'\n", data[b]);
            }
            unsigned c = ++b;
            while(b < data.size() && WcharToAsc(data[b]) != ',') ++b;
            
            ucs4string s = data.substr(c, b-c);
            
            const ucs4string &name = Symbols.GetRev(16).find(person)->second;
            
            ctstring key;
            for(unsigned a=0; a<s.size(); ++a)
                if(s.compare(a, name.size(), name) == 0)
                {
                    key += person;
                    a += name.size() - 1;
                }
                else
                    key += getchronochar(s[a], cset_12pix);
            
            tmp.data[key] = person;
        }
        
        AddForm(tmp);
    }
}

void Conjugatemap::Work(ctstring &s, formit fit)
{
    form &form = *fit;

    datamap_t::const_iterator i;
    for(i=form.data.begin(); i!=form.data.end(); ++i)
    {
        for(unsigned a=0; a < s.size(); )
        {
            unsigned b = s.find(i->first, a);
            if(b == s.npos) break;
            
            if(!form.used)
            {
                unsigned byte = 0x2FF;
                for(;; --byte)
                {
                    /* If this byte is used... */
                    if(getucs4(byte, cset_12pix) != ilseq) continue;

                    bool used = false;
                    
                    const ConfParser::ElemVec& elems = GetConf("font", "typeface").Fields();
                    for(unsigned a=0; a<elems.size(); a += 6)
                    {
                        unsigned begin = elems[a+3];
                        unsigned end   = elems[a+4];
                        if(byte >= begin && byte < end)
                        {
                            used = true;
                            break;
                        }
                    }
                    
                    if(used) continue;
                    
                    for(formlist::const_iterator j=forms.begin(); j!=forms.end(); ++j)
                        if(j->used
                        && j->prefix == byte)
                        {
                            used = true;
                            break;
                        }
                    if(!used) break;
                }
                
                form.used   = true;
                form.prefix = byte;

#if 0                
                fprintf(stderr, " \n  Assigned ControlCode 0x%04X for %s ",
                    form.prefix,
                    WstrToAsc(form.func).c_str());
#endif
                
                charmap[form.prefix] = fit;
            }
            
            ctstring tmp;
            tmp += form.prefix; // conjugater id
            tmp += i->second;   // character id
            
            // a = b + i->first.size();
            
            s.erase(b, i->first.size());
            s.insert(b, tmp);
            a = b + tmp.size();
        }
    }
}

bool Conjugatemap::IsConjChar(ctchar c) const
{
    return charmap.find(c) != charmap.end();
}

void Conjugatemap::RedefineConjChar(ctchar was, ctchar is)
{
    charmap_t::iterator i = charmap.find(was);
    if(i != charmap.end())
    {
        formit tmp = i->second;
        tmp->prefix = is;
        charmap.erase(i);
        charmap[is] = tmp;
    }
}

unsigned Conjugatemap::GetMaxWidth(ctchar c) const
{
    charmap_t::const_iterator i = charmap.find(c);
    if(i == charmap.end())return 0;
    return i->second->maxwidth;
}

Conjugatemap::Conjugatemap(const insertor &ins)
{
    Load(ins);
    fprintf(stderr, "Built conjugator-map\n");
}

void Conjugatemap::Work(ctstring &s)
{
    for(formit i = forms.begin(); i != forms.end(); ++i)
        Work(s, i);
}

#include "ctinsert.hh"
#include "compiler.hh"

namespace
{
    const SubRoutine GetConjugateCode(const Conjugatemap *const Conjugater)
    {
        SubRoutine result;
        SNEScode &code = result.code;

        SNEScode::RelativeBranch branchEnd = code.PrepareRelativeBranch();
        SNEScode::RelativeBranch branchOut = code.PrepareRelativeBranch();
        
        const Conjugatemap::formlist &forms = Conjugater->GetForms();
        
        bool first=true;
        
        Conjugatemap::formlist::const_iterator i;
        for(i=forms.begin(); i!=forms.end(); ++i)
        {
            if(!i->used) continue;
            
            SNEScode::RelativeBranch branchSkip = code.PrepareRelativeBranch();
            code.Set16bit_M();
            code.EmitCode(0xC9, i->prefix&255, i->prefix>>8); //cmp a, prefix
            code.EmitCode(0xD0, 0);                           //bne
            branchSkip.FromHere();
            
            if(true)
            {
                const ucs4string &funcname = i->func;

                code.EmitCode(0x22, 0,0,0);
                result.requires[funcname].insert(code.size() - 3);
                
                if(first)
                {
                    branchOut.ToHere();
                    code.BitnessUnknown();
                    code.Set16bit_M();
                    
                    // increment the pointer (skip the name printing)
                    code.EmitCode(0xE6, 0x31);

                    code.EmitCode(0x80, 0); // bra - jump out
                    branchEnd.FromHere();
                    
                    first = false;
                }
                else
                {
                    code.EmitCode(0x80, 0); // bra - jump out
                    branchOut.FromHere();
                }
            }
            branchSkip.ToHere();
            branchSkip.Proceed();
        }

        code.Set8bit_M();
        SNEScode::FarToNearCall call = code.PrepareFarToNearCall();
        call.Proceed(DialogDrawFunctionAddr | 0xC00000);   /* call */
        
        branchEnd.ToHere();
        code.BitnessUnknown();
        
        // Ready to return
        code.Set8bit_M();
        code.EmitCode(0x6B);       //rtl
        
        branchEnd.Proceed();
        branchOut.Proceed();

        // This function will be called from.
        code.AddCallFrom(ConjugatePatchAddress | 0xC00000);
        
        return result;
    }
}

void insertor::GenerateConjugatorCode()
{
    string functionfn = WstrToAsc(GetConf("conjugator", "codefn"));
    ucs4string ConjFuncName  = GetConf("conjugator", "funcname");
    
    FILE *fp = fopen(functionfn.c_str(), "rt");
    if(!fp) return;
    
    fprintf(stderr, "Compiling %s...\n", functionfn.c_str());
    FunctionList Functions = Compile(fp);
    fclose(fp);
    
    SubRoutine conjugator = GetConjugateCode(Conjugater);
    Functions.Define(ConjFuncName, conjugator);

    Functions.RequireFunction(ConjFuncName);
    
    LinkAndLocate(Functions);

    // SEP+JSR takes 5 bytes. We overwrote it with 4 bytes.
    PlaceByte(0xEA, ConjugatePatchAddress + 4); // NOP
}

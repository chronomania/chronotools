#include <cstdio>
#include <cstdarg>

#include "conjugate.hh"
#include "settings.hh"
#include "symbols.hh"
#include "miscfun.hh"
#include "ctcset.hh"
#include "config.hh"
#include "space.hh"

using namespace std;

void Conjugatemap::Load()
{
    const ConfParser::ElemVec& elems = GetConf("conjugator", "setup").Fields();
    for(unsigned a=0; a<elems.size(); a += 2)
    {
        form tmp;
        const wstring &func = elems[a];
        const wstring &data = elems[a+1];
        
        tmp.func   = func;
        tmp.used   = false;
        tmp.prefix = 0;
        
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
            
            wstring s = data.substr(c, b-c);
            
            const wstring &name = Symbols.GetRev(16).find(person)->second;
            
            ctstring key;
            for(unsigned a=0; a<s.size(); ++a)
                if(s.compare(a, name.size(), name) == 0)
                {
                    key += person;
                    a += name.size() - 1;
                }
                else
                    key += getchronochar(s[a]);
            
            tmp.data[key] = person;
        }
        
        AddForm(tmp);
    }
}

void Conjugatemap::Work(ctstring &s, form &form)
{
    static const vector<ctchar> AllowedBytes = GetConjugateBytesList();
    
    datamap_t::const_iterator i;
    for(i=form.data.begin(); i!=form.data.end(); ++i)
    {
        for(unsigned a=0; a < s.size(); )
        {
            unsigned b = s.find(i->first, a);
            if(b == s.npos) break;
            
            if(!form.used)
            {
                unsigned ind = 0;
                for(formlist::const_iterator j=forms.begin(); j!=forms.end(); ++j)
                    if(j->used)
                        ++ind;
                
                if(ind >= AllowedBytes.size())
                {
                    fprintf(stderr,
                        "\n"
                        "Error: Too many different conjugations used.\n"
                        "Only %u bytes defined in configuration!\n",
                            AllowedBytes.size()
                           );
                }
                
                form.used   = true;
                form.prefix = AllowedBytes[ind];
                
                fprintf(stderr, "  Assigned 0x%04X for %s\n",
                    form.prefix,
                    WstrToAsc(form.func).c_str());
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

Conjugatemap::Conjugatemap()
{
    Load();
    fprintf(stderr, "Built conjugator-map\n");
}

void Conjugatemap::Work(ctstring &s)
{
    for(formlist::iterator
        i = forms.begin();
        i != forms.end();
        ++i)
    {
        Work(s, *i);
    }
}

class Conjugatemap Conjugatemap;

#include "ctinsert.hh"
#include "compiler.hh"

namespace
{
    const SubRoutine GetConjugateCode()
    {
        SubRoutine result;
        SNEScode &code = result.code;

        SNEScode::RelativeBranch branchEnd = code.PrepareRelativeBranch();
        SNEScode::RelativeBranch branchOut = code.PrepareRelativeBranch();
        
        const Conjugatemap::formlist &forms = Conjugatemap.GetForms();
        
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
                const wstring &funcname = i->func;

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
    wstring ConjFuncName  = GetConf("conjugator", "funcname");
    
    FILE *fp = fopen(functionfn.c_str(), "rt");
    if(!fp) return;
    
    fprintf(stderr, "Compiling %s...\n", functionfn.c_str());
    FunctionList Functions = Compile(fp);
    fclose(fp);
    
    SubRoutine conjugator = GetConjugateCode();
    Functions.Define(ConjFuncName, conjugator);

    Functions.RequireFunction(ConjFuncName);
    
    LinkAndLocate(Functions);
}

const vector<ctchar> GetConjugateBytesList()
{
    vector<ctchar> result;
    const ConfParser::ElemVec& elems = GetConf("conjugator", "bytes").Fields();
    for(unsigned a=0; a<elems.size(); ++a)
        result.push_back(elems[a].IField);
    return result;
}

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
        const string func = WstrToAsc(elems[a]);
        
        const string data = WstrToAsc(elems[a+1]);
        
        tmp.func   = func;
        tmp.used   = false;
        tmp.prefix = 0;
        
        for(unsigned b=0; b<data.size(); ++b)
        {
            if(data[b] == ' ' || data[b] == '\n'
            || data[b] == '\r' || data[b] == '\n') continue;
            
            const char *name = "...";
            switch(data[b])
            {
                case 'c': name="Crono"; break;
                case 'm': name="Marle"; break;
                case 'l': name="Lucca"; break;
                case 'r': name="Robo"; break;
                case 'f': name="Frog"; break;
                case 'a': name="Ayla"; break;
                case 'u': name="Magus"; break;
                case 'e': name="Epoch"; break;
                case '1': name="[member1]"; break;
                case '2': name="[member2]"; break;
                case '3': name="[member3]"; break;
                default:
                    fprintf(stderr, "In configuration: Unknown person '%c'\n", data[b]);
            }
            unsigned c = ++b;
            while(b < data.size() && data[b] != ',') ++b;
            
            string s = data.substr(c, b-c);

            unsigned char person = Symbols[16].find(name)->second;
            string key = str_replace(name, person, s);
            for(unsigned a=0; a<key.size(); ++a)
                if(key[a] != person)
                    key[a] = getchronochar(key[a]);

#if 0
            fprintf(stderr, "Key '%s'(%s) = '%s' (%02X)\n",
                key.c_str(), s.c_str(), name, person);
#endif
            tmp.data[key] = person;
        }
        
        AddForm(tmp);
    }
}

void Conjugatemap::Work(string &s, form &form)
{
    vector<unsigned> AllowedBytes;
    
    { const ConfParser::ElemVec& elems = GetConf("conjugator", "bytes").Fields();
    for(unsigned a=0; a<elems.size(); ++a)
        AllowedBytes.push_back(elems[a]); }
    
    datamap_t::const_iterator i;
    for(i=form.data.begin(); i!=form.data.end(); ++i)
        for(unsigned a=0; a < s.size(); )
        {
            unsigned b = s.find(i->first, a);
            if(b == s.npos) break;
            
            if(!form.used)
            {
                unsigned ind = 0;
                for(formlist::const_iterator j=forms.begin(); j!=forms.end(); ++j)
                    if(j->used) ++ind;
                
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
            }
            
            string tmp;
            tmp += (char)form.prefix;
            tmp += (char)i->second;
            
            // a = b + i->first.size();
            
            s.erase(b, i->first.size());
            s.insert(b, tmp);
            a = b + tmp.size();
        }
}

Conjugatemap::Conjugatemap()
{
    Load();
    fprintf(stderr, "Built conjugator-map\n");
}

void Conjugatemap::Work(string &s, const string &plaintext)
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
            code.Set8bit_M();
            code.EmitCode(0xC9, i->prefix); //cmp a, *
            code.EmitCode(0xD0, 0);         //bne
            branchSkip.FromHere();
            
            if(true)
            {
                const string &funcname = i->func;

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
        
        SNEScode::FarToNearCall call = code.PrepareFarToNearCall();
        call.Proceed(0xC25DC4);   /* call */
        
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

void insertor::GenerateCode()
{
    string functionfn = WstrToAsc(GetConf("conjugator", "codefn"));
    
    FILE *fp = fopen(functionfn.c_str(), "rt");
    if(!fp) return;
    
    fprintf(stderr, "Compiling %s...\n", functionfn.c_str());
    FunctionList Functions = Compile(fp);
    fclose(fp);
    
    SubRoutine conjugator = GetConjugateCode();
    Functions.Define("conjugator", conjugator);
    
    Functions.RequireFunction("conjugator");
    
    vector<SNEScode> codeblobs;
    vector<string>   funcnames;
    
    for(FunctionList::functions_t::const_iterator
        i = Functions.functions.begin();
        i != Functions.functions.end();
        ++i)
    {
        // Omit nonrequired functions.
        if(!i->second.second) continue;
        
        codeblobs.push_back(i->second.first.code);
        funcnames.push_back(i->first);
    }
        
    vector<freespacerec> blocks(codeblobs.size());
    for(unsigned a=0; a<codeblobs.size(); ++a)
        blocks[a].len = codeblobs[a].size();
    
    freespace.OrganizeToAnyPage(blocks);
    
    for(unsigned a=0; a<codeblobs.size(); ++a)
    {
        unsigned addr = blocks[a].pos;
        codeblobs[a].YourAddressIs(addr);
        fprintf(stderr, "  Function %s (%u bytes) will be placed at %02X:%04X\n",
            funcnames[a].c_str(),
            codeblobs[a].size(),
            0xC0 | (addr>>16),
            addr & 0xFFFF);
    }
    
    // All of them are now placed somewhere.
    // Link them!
    
    for(unsigned a=0; a<codeblobs.size(); ++a)
    {
        FunctionList::functions_t::const_iterator i = Functions.functions.find(funcnames[a]);
        
        const SubRoutine::requires_t &req = i->second.first.requires;
        for(SubRoutine::requires_t::const_iterator
            j = req.begin();
            j != req.end();
            ++j)
        {
            // Find the address of the function we're requiring
            unsigned req_addr = NOWHERE;
            for(unsigned b=0; b<funcnames.size(); ++b)
                if(funcnames[b] == j->first)
                {
                    req_addr = codeblobs[b].GetAddress() | 0xC00000;
                    break;
                }
            
            for(set<unsigned>::const_iterator
                k = j->second.begin();
                k != j->second.end();
                ++k)
            {
                codeblobs[a][*k + 0] = req_addr & 255;
                codeblobs[a][*k + 1] = (req_addr >> 8) & 255;
                codeblobs[a][*k + 2] = req_addr >> 16;
            }
        }
    }
    
    // They have now been linked.
    
    codes.insert(codes.begin(), codeblobs.begin(), codeblobs.end());
}

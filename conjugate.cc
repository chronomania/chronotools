#include <cstdio>
#include <cstdarg>

#include "conjugate.hh"
#include "symbols.hh"
#include "miscfun.hh"
#include "ctcset.hh"
#include "space.hh"

using namespace std;

namespace
{
    const char functionfn[]            = "ct.code";
    const unsigned char AllowedBytes[] = {0xFF,0xFE,0xFC,0xFB,0xFA};
}

void Conjugatemap::Load()
{
    form tmp;
    tmp.data = CreateMap
        ( "Cronon", "Marlen", "Luccan", "Lucan",
          "Robon", "Frogn", "Frogin", "Aylan", "Maguksen",
          "Magusin", "Epochin",
          "[member1]:n", "[member2]:n", "[member3]:n",
          0 );
    tmp.type = Cnj_N;
    AddForm(tmp);
    
    tmp.data = CreateMap
        ( "Cronoa", "Marlea", "Luccaa",
          "Roboa", "Frogia", "Froggia",
          "Aylaa", "Magusta", "Maguksea", "Magusia", "Epochia",
          "[member1]:a", "[member2]:a", "[member3]:a",
          "[member1]:ä", "[member2]:ä", "[member3]:ä",
          0 );
    tmp.type = Cnj_A;
    AddForm(tmp);
    
    tmp.data = CreateMap
        ( "Cronolla", "Marlella", "Luccalla", "Lucalla",
          "Robolla", "Frogilla", "Aylalla", "Maguksella",
          "Magusilla", "Epochilla", 
          "[member1]:lla", "[member2]:lla", "[member3]:lla",
          "[member1]:llä", "[member2]:llä", "[member3]:llä",
          0 );
    tmp.type = Cnj_LLA;
    AddForm(tmp);
    
    tmp.data = CreateMap
        ( "Cronolle", "Marlelle", "Luccalle", "Lucalle",
          "Robolle", "Frogille", "Aylalle", "Magukselle",
          "Magusille", "Epochille", 
          "[member1]:lle", "[member2]:lle", "[member3]:lle",
          0 );
    tmp.type = Cnj_LLE;
    AddForm(tmp);
    
    tmp.data = CreateMap
        ( "Cronosta", "Marlesta", "Luccasta", "Lucasta",
          "Robosta", "Frogista", "Aylasta", "Maguksesta",
          "Magusista", "Epochista", 
          "[member1]:sta", "[member2]:sta", "[member3]:sta",
          "[member1]:stä", "[member2]:stä", "[member3]:stä",
          0 );
    tmp.type = Cnj_STA;
    AddForm(tmp);
}

void Conjugatemap::AddData(datamap_t &target, const string &s) const
{
    const map<string, char> &symbols16 = Symbols[16];
    const char *name = "...";
    /* Simple method to see which character are we talking about.      */
    /* Modify it if the first character isn't enough in your language. */
    switch(s[0])
    {
        case 'C': name = "Crono"; break;
        case 'L': name = "Lucca"; break;
        case 'M':
            switch(s[2])
            {
                case 'g': name = "Magus"; break;
                case 'r': name = "Marle"; break;
            }
            break;
        case '[':
            switch(s[7])
            {
                case '1': name = "[member1]"; break;
                case '2': name = "[member2]"; break;
                case '3': name = "[member3]"; break;
            }
            break;
        case 'R': name = "Robo"; break;
        case 'F': name = "Frog"; break;
        case 'A': name = "Ayla"; break;
        case 'E': name = "Epoch"; break;
        
        // Nadia can't be renamed, so it
        // does not need to be taken care of.
    }
    unsigned char person = symbols16.find(name)->second;
    string key = str_replace(name, person, s);
    for(unsigned a=0; a<key.size(); ++a)
        if((key[a] >= 'a' && key[a] <= 'z')
        || (key[a] >= 'A' && key[a] <= 'Z')
        || (key[a] == ':' || key[a] == 'ä'))
            key[a] = getchronochar(key[a]);
#if 0
    fprintf(stderr, "Key '%s'(from '%s') = '%s' (%02X)\n",
        key.c_str(), s.c_str(), name, person);
#endif
    target[key] = person;
}

Conjugatemap::datamap_t Conjugatemap::CreateMap(const char *word, ...) const
{
    datamap_t result;
    va_list ap;
    va_start(ap, word);
    while(word)
    {
        AddData(result, word);
        word = va_arg(ap, const char *);
    }
    va_end(ap);
    return result;
}

void Conjugatemap::Work(string &s, const form &form)
{
    datamap_t::const_iterator i;
    for(i=form.data.begin(); i!=form.data.end(); ++i)
    {
        for(unsigned a=0; a < s.size(); )
        {
            unsigned b = s.find(i->first, a);
            if(b == s.npos) break;
            
            unsigned char prefix;
            
            map<CnjType, unsigned char>::const_iterator k;
            k = prefixes.find(form.type);
            if(k != prefixes.end())
                prefix = k->second;
            else
            {
                unsigned ind = prefixes.size();
                
                if(ind >= sizeof(AllowedBytes))
                {
                    fprintf(stderr,
                        "\n"
                        "Error: Too many different conjugations used.\n"
                        "Only %u bytes fit!\n",
                            sizeof(AllowedBytes)
                           );
                }
                
                prefixes[form.type] = prefix = AllowedBytes[ind];
            }
            
            string tmp;
            tmp += (char)prefix;
            tmp += (char)i->second;
            
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

void Conjugatemap::Work(string &s, const string &plaintext)
{
    for(list<form>::const_iterator
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
        
        const map<CnjType, unsigned char> &prefixes = Conjugatemap.GetPref();
        
        bool first=true;
        
        map<CnjType, unsigned char>::const_iterator i;
        for(i=prefixes.begin(); i!=prefixes.end(); ++i)
        {
            SNEScode::RelativeBranch branchSkip = code.PrepareRelativeBranch();
            code.Set8bit_M();
            code.AddCode(0xC9, i->second); //cmp a, *
            code.AddCode(0xD0, 0);         //bne
            branchSkip.FromHere();
            
            if(true)
            {
                const char *funcname = "?";
                switch(i->first)
                {
                    case Cnj_N:   funcname = "Do_N"; break;
                    case Cnj_A:   funcname = "Do_A"; break;
                    case Cnj_LLA: funcname = "Do_LLA"; break;
                    case Cnj_LLE: funcname = "Do_LLE"; break;
                    case Cnj_STA: funcname = "Do_STA"; break;
                }
                
                code.AddCode(0x22, 0,0,0);
                result.requires[funcname].insert(code.size() - 3);
                
                if(first)
                {
                	branchOut.ToHere();
                	code.BitnessUnknown();
                    code.Set16bit_M();
                    
                    // increment the pointer (skip the name printing)
                    code.AddCode(0xE6, 0x31);

                    code.AddCode(0x80, 0); // bra - jump out
                    branchEnd.FromHere();
                    
                    first = false;
                }
                else
                {
                    code.AddCode(0x80, 0); // bra - jump out
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
        code.AddCode(0x6B);       //rtl
        
        branchEnd.Proceed();
        branchOut.Proceed();

        // This function will be called from.
        code.AddCallFrom(0xC258C2);
        
        return result;
    }
}

void insertor::GenerateCode()
{
    FILE *fp = fopen(functionfn, "rt");
    if(!fp) return;
    
    fprintf(stderr, "Compiling %s...\n", functionfn);
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

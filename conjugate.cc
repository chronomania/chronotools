#include <cstdio>
#include <cstdarg>

#include "conjugate.hh"
#include "symbols.hh"
#include "miscfun.hh"
#include "ctcset.hh"

using namespace std;

namespace
{
    const unsigned char AllowedBytes[] = {0xFF,0xFE,0xFC,0xFB,0xFA};
}

void Conjugatemap::Load()
{
    form tmp;
    tmp.data = CreateMap
        ( "Cronon", "Marlen", "Luccan", "Lucan",
          "Robon", "Frogin", "Aylan", "Maguksen",
          "Magusin", "Epochin", 0 );
    tmp.type = Cnj_N;
    AddForm(tmp);
    
    tmp.data = CreateMap
        ( "Cronoa", "Marlea", "Luccaa",
          "Roboa", "Frogia", "Froggia",
          "Aylaa", "Magusta", "Epochia", 0 );
    tmp.type = Cnj_A;
    AddForm(tmp);
    
    tmp.data = CreateMap
        ( "Cronolla", "Marlella", "Luccalla", "Lucalla",
          "Robolla", "Frogilla", "Aylalla", "Maguksella",
          "Magusilla", "Epochilla", 0 );
    tmp.type = Cnj_LLA;
    AddForm(tmp);
    
    tmp.data = CreateMap
        ( "Cronolle", "Marlelle", "Luccalle", "Lucalle",
          "Robolle", "Frogille", "Aylalle", "Magukselle",
          "Magusille", "Epochille", 0 );
    tmp.type = Cnj_LLE;
    AddForm(tmp);
    
    tmp.data = CreateMap
        ( "Cronosta", "Marlesta", "Luccasta", "Lucasta",
          "Robosta", "Frogista", "Aylasta", "Maguksesta",
          "Magusista", "Epochista", 0 );
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
        case 'M': name = s[2]=='g' ? "Magus" : "Marle"; break;
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
        if(key[a] >= 'a' && key[a] <= 'z')
            key[a] = getchronochar(key[a]);
    
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
                        "Only %u bytes available in the font!\n",
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
    fprintf(stderr, "Built conjugater-map\n");
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

static const SNEScode GetTestCode()
{
    SNEScode code;
    
    // C2:5DC4 is referenced from:
    //      C2:5C12 
    //      C2:58C4
    
#define TRY_DOUBLE 1  // Print two characters instead of one?
#define MODIFY 1      // Modify the character to a bit?

#if TRY_DOUBLE && MODIFY
    // save A
    code.AddCode(0x48);       //pha
#endif

#if MODIFY
    // modify the character
    code.Set8bit();
    code.AddCode(0xA9, 0xA8); //lda a, $A8
    code.AddCode(0x85, 0x35); //sta [$00:D+$35]
#endif

#if TRY_DOUBLE
    // prepare the framework for a farcall to a near routine
    void *call = code.PrepareFarToNearCall();
    // overwrites a and pushes stuff

    // call back the routine
    code.Set16bit();
    code.AddCode(0xA5, 0x35); //lda [$00:D+$35]
    
    code.ProceedFarToNearCall(call, 0xC25DC8);

#if MODIFY
    // restore A
    code.AddCode(0x68);       //pla

    // restore character
    code.Set8bit();
    code.AddCode(0x85, 0x35); //sta [$00:D+$35]
#endif
#endif

    // be ready to return to the routine
    code.Set16bit();
    code.AddCode(0xA5, 0x35); //lda [$00:D+$35]
    code.AddCode(0x6B);       //rtl
    
    code.AddCallFrom(0xC25DC4);
    
    return code;
}


void insertor::GenerateCode()
{
#if 1
    SNEScode code = GetTestCode();
    
    unsigned address = freespace.FindFromAnyPage(code.size());

    code.YourAddressIs(address);
    
    codes.push_back(code);
#endif
}

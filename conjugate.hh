#include <map>
#include <string>
#include <list>

using std::map;
using std::string;
using std::list;

enum CnjType
{
    Cnj_N,
    Cnj_A,
    Cnj_LLA,
    Cnj_LLE,
    Cnj_STA
};

class Conjugatemap
{
public:
    Conjugatemap();

    void Work(string &s, const string &plaintext);

private:
    typedef map<string, unsigned char> datamap_t;
    struct form
    {
        datamap_t   data;
        CnjType     type;
    };
    
    map<CnjType, unsigned char> prefixes;
    
    list<form> forms;
    void AddForm(const form &form) { forms.push_back(form); }
    void AddData(datamap_t &target, const string &s) const;
    datamap_t CreateMap(const char *word, ...) const;
    void Work(string &s, const form &form);
    void Load();
    
    bool IsUsed(CnjType c) const { return prefixes.find(c)!=prefixes.end(); }
    unsigned char GetByte(CnjType c) const { return prefixes.find(c)->second; }
};

extern Conjugatemap Conjugatemap;

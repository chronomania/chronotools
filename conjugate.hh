#include <map>
#include <list>

using std::map;
using std::list;

#include "wstring.hh"

class Conjugatemap
{
public:
    Conjugatemap();

    void Work(string &s);

    typedef map<string, unsigned char> datamap_t;

    struct form
    {
        datamap_t     data;
        wstring       func;
        bool          used;
        unsigned char prefix;
    };
    
    typedef list<form> formlist;

private:
    formlist forms;
    void AddForm(const form &form) { forms.push_back(form); }
    void Work(string &s, form &form);
    void Load();
public:
    const formlist &GetForms() const { return forms; }
};

extern Conjugatemap Conjugatemap;

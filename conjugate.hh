#include <map>
#include <list>
#include <vector>

using std::map;
using std::list;
using std::vector;

#include "wstring.hh"
#include "ctcset.hh"

class Conjugatemap
{
public:
    Conjugatemap();

    void Work(ctstring &s);

    typedef map<ctstring, ctchar> datamap_t;

    struct form
    {
        datamap_t     data;
        wstring       func;
        bool          used;
        ctchar        prefix;
    };
    
    typedef list<form> formlist;

private:
    formlist forms;
    void AddForm(const form &form) { forms.push_back(form); }
    void Work(ctstring &s, form &form);
    void Load();
public:
    const formlist &GetForms() const { return forms; }
};

extern Conjugatemap Conjugatemap;

const vector<ctchar> GetConjugateBytesList();

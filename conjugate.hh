#ifndef bqtConjugateHH
#define bqtConjugateHH

#include <list>
#include <vector>

using std::list;
using std::vector;

#include "wstring.hh"
#include "ctcset.hh"
#include "hash.hh"

class Conjugatemap
{
public:
    explicit Conjugatemap(const class insertor &ins);

    void Work(class insertor &ins, ctstring &s);

    typedef hash_map<ctstring, ctchar> datamap_t;

    struct form
    {
        datamap_t     data;
        ucs4string    func;
        bool          used;
        ctchar        prefix;
        unsigned      maxwidth;
    };
    
    typedef list<form> formlist;

private:
    formlist forms;
    
    typedef formlist::iterator formit;

    typedef hash_map<ctchar, formit> charmap_t;
    
    charmap_t charmap;

    void AddForm(const form &form) { forms.push_back(form); }
    void Work(class insertor &ins, ctstring &s, formit fit);
    void Load(const class insertor &ins);
public:
    const formlist &GetForms() const { return forms; }
    
    bool IsConjChar(ctchar c) const;
    void RedefineConjChar(ctchar was, ctchar is);
    
    unsigned GetMaxWidth(ctchar c) const; // Max width caused by the conjugation char
};

#endif

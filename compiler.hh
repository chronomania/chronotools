#include <cstdio>
#include <utility>
#include <map>
#include <set>

#include "snescode.hh"
#include "wstring.hh"

using namespace std;

struct SubRoutine
{
    // code
    SNEScode code;
    
    // funcname -> positions where called
    typedef map<wstring, set<unsigned> > requires_t;
    requires_t requires;
    
    void CallSub(const wstring &name);
};

struct FunctionList
{
    // funcname -> pair<function, requiredflag>
    typedef map<wstring, pair<SubRoutine, bool> > functions_t;
    functions_t functions;

    void Define(const wstring &name, const SubRoutine &sub);
    void RequireFunction(const wstring &name);
};

const FunctionList Compile(FILE *fp);

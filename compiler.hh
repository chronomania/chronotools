#include <cstdio>
#include <string>
#include <utility>
#include <map>
#include <set>

#include "snescode.hh"

using namespace std;

struct SubRoutine
{
    // code
    SNEScode code;
    
    // funcname -> positions where called
    typedef map<string, set<unsigned> > requires_t;
    requires_t requires;
};

struct FunctionList
{
    // funcname -> pair<function, requiredflag>
    typedef map<string, pair<SubRoutine, bool> > functions_t;
    functions_t functions;

    void Define(const string &name, const SubRoutine &sub);
    void RequireFunction(const string &name);
};

const FunctionList Compile(FILE *fp);

#include <map>
#include <string>
#include <vector>

#include "wstring.hh"
#include "space.hh"

using namespace std;

struct stringoffsdata
{
    string   str;
    unsigned offs;
};
class stringoffsmap: public vector<stringoffsdata>
{
public:
    // parasite -> host
    typedef map<unsigned, unsigned> neederlist_t;
    neederlist_t neederlist;
    
    void GenerateNeederList();
};

class insertor
{
    struct stringdata
    {
        string str;
        enum { zptr8, zptr16, fixed } type;
        unsigned width; // used if type==fixed;
    };
    // Address -> string
    typedef map<unsigned, stringdata> stringmap;
    stringmap strings;
    
    vector<string> dict;
    unsigned dictaddr, dictsize;

public:
    freespacemap freespace;
    
    void LoadFile(FILE *fp);

    void DictionaryCompress();
    void GeneratePatches();

    string DispString(const string &s) const;
    
private:
    void WriteDictionary(class ROM &ROM);
    void WriteStrings(class ROM &ROM);
    void Write8pixfont(class ROM &ROM);
    void Write12pixfont(class ROM &ROM);
    
    void ApplyDictionary();
    void RebuildDictionary();
    
    // Get list of pages having zstrings
    const set<unsigned> GetZStringPageList() const;
    // Get zstrings of page
    const stringoffsmap GetZStringList(unsigned pagenum) const;

    unsigned CalculateScriptSize() const;
};

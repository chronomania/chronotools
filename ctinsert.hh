#include <map>
#include <string>
#include <vector>

#include "wstring.hh"
#include "space.hh"
#include "fonts.hh"

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
        enum { zptr8, zptr12, fixed } type;
        unsigned width; // used if type==fixed;
    };
    // Address -> string
    typedef map<unsigned, stringdata> stringmap;
    stringmap strings;
    
    vector<string> dict;
    unsigned dictaddr, dictsize;
    
    Font8data Font8;
    Font12data Font12;

public:
    freespacemap freespace;
    
    void LoadFile(FILE *fp);
    void LoadFont8(const string &fn) { Font8.Load(fn); }
    void LoadFont12(const string &fn) { Font12.Load(fn); }

    void DictionaryCompress();

    string DispString(const string &s) const;
    
    unsigned GetFont12width(unsigned char chronoch) const
    { return Font12.GetWidth(chronoch-0xA0); }
    
    void PatchROM(class ROM &ROM);

private:
    void WriteDictionary(class ROM &ROM);
    void WriteStrings(class ROM &ROM);
    void Write8pixfont(class ROM &ROM);
    void Write12pixfont(class ROM &ROM);
    void WriteCode(class ROM &ROM);
    
    void ApplyDictionary();
    void RebuildDictionary();
    
    // Get list of pages having zstrings
    const set<unsigned> GetZStringPageList() const;
    // Get zstrings of page
    const stringoffsmap GetZStringList(unsigned pagenum) const;

    unsigned CalculateScriptSize() const;
};

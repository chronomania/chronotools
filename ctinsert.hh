#include <map>
#include <string>
#include <vector>

#include "wstring.hh"
#include "space.hh"
#include "rom.hh"

using namespace std;

class insertor
{
    class stringdata
    {public:
        string str;
        enum { zptr8, zptr16, fixed } type;
        unsigned width; // used if type==fixed;
    };

    typedef map<unsigned, stringdata> stringmap;
    stringmap strings;

    vector<string> dict;
    unsigned dictaddr, dictsize;

public:
    freespacemap freespace;
    
    void LoadFile(FILE *fp);

    string DispString(const string &s) const;
    void MakeDictionary();
    
    void WriteDictionary(ROM &ROM);
    void WriteStrings(ROM &ROM);
    void Write8pixfont(ROM &ROM);
    void Write12pixfont(ROM &ROM);
    
    void GeneratePatches();
};


#include <map>
#include <string>
#include <vector>

#include "wstring.hh"
#include "space.hh"

using namespace std;

class insertor
{
    map<string, char> symbols2, symbols8, symbols16;
    
    class stringdata
    {public:
        string str;
        enum { zptr8, zptr16, fixed } type;
        unsigned width; // used if type==fixed;
    };
    class substringrec
    {
        unsigned count;
        double importance;
    public:
        substringrec() : count(0), importance(0) {}
        void mark(double imp) { ++count; importance += 1.0/imp; }
        unsigned getcount() const { return count; }
        double getimportance() const { return 1.0/(importance / count);}
    };

    typedef map<unsigned, stringdata> stringmap;
    stringmap strings;

    vector<string> dict, replacedict;
    unsigned dictaddr, dictsize;

public:
    freespacemap freespace;
    
    void LoadSymbols();
    void LoadFile(FILE *fp);

    string DispString(const string &s) const;
    void MakeDictionary();
    
    inline void WriteByte(vector<unsigned char> &ROM,
                          vector<bool> &Touched,
                          unsigned offs, unsigned char value)
    {
        ROM[offs] = value;
        Touched[offs] = true;
    }
    
    unsigned WritePPtr(vector<unsigned char> &ROM,
                       vector<bool> &Touched,
                       unsigned pointeraddr,
                       const string &string);

    unsigned WriteZPtr(vector<unsigned char> &ROM,
                       vector<bool> &Touched,
                       unsigned pointeraddr,
                       const string &string,
                       unsigned spaceptr=NOWHERE);
    
    void Write8pixfont(vector<unsigned char> &ROM,
                       vector<bool> &Touched);
    
    void Write12pixfont(vector<unsigned char> &ROM,
                        vector<bool> &Touched);
    
    void WriteROM();
};


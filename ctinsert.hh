#ifndef bqtCTinsertHH
#define bqtCTinsertHH

#include <list>
#include <string>
#include <vector>
#include <utility>

#include "wstring.hh"
#include "space.hh"
#include "fonts.hh"
#include "ctcset.hh"

#include "o65linker.hh"
#include "refer.hh"

using namespace std;

const string DispString(const ctstring &s, unsigned symbols_type=16);

class insertor
{
public:
    void LoadFile(FILE *fp);
    void LoadFont8(const string &fn) { Font8.Load(fn); }
    void LoadFont8v(const string &fn) { Font8v.Load(fn); }
    void LoadFont12(const string &fn) { Font12.Load(fn); }
    
    void LoadImage(const string &fn, unsigned address);

    void WriteEverything();
    void DictionaryCompress();

    unsigned GetFont12width(ctchar chronoch) const;
    
    void ClearROM(class ROM &ROM) const;
    void PatchROM(class ROM &ROM) const;

    void ReportFreeSpace();
    void ReorganizeFonts();
    
    insertor();
    ~insertor();
    
    // Shouldn't be public, but StringReceipt and Conjugatemap need this.
    O65linker objects;

private:
    struct stringdata
    {
        ctstring str;
        typedef enum { zptr8, zptr12, fixed, item, tech, monster } strtype;
        strtype type;
        unsigned width; // used if type==fixed;
        unsigned address;
        
        unsigned ref_id; // used if zptr8 or zptr12
    };
    
    typedef list<stringdata> stringlist;
    stringlist strings;
    
    typedef vector<list<ReferMethod> > referlist;
    referlist refers;
    
    vector<ctstring> dict;
    
    Font8data Font8;
    Font8vdata Font8v;
    Font12data Font12;
    
    void PlaceData(const vector<unsigned char>&, unsigned address, const string& reason="");
    void PlaceByte(unsigned char byte, unsigned address, const string& reason="");

    freespacemap freespace;
    
    class Conjugatemap *Conjugater;
    
    void WriteImages();
    void WriteFonts();
    void WriteVWF12();
    void WriteVWF8();
    void WriteFont8();
    void WriteConjugator();
    void WriteUserCode();
    void WriteDictionary();
    void WriteStrings();
    void WriteFixedStrings();
    void WriteOtherStrings();
    void WriteRelocatedStrings();
    void WriteStringTable(stringdata::strtype type,
                          const string& tablename,
                          const string& what);
    
    /* Used to mark free space in the ROM because of code that is no longer used. */
    void ObsoleteCode(unsigned addr, unsigned bytes, bool barrier=false);
    
    void ApplyDictionary();
    void RebuildDictionary();
    
    const ctstring ParseScriptEntry(const ucs4string &input, const stringdata &model);
    const ctstring WrapDialogLines(const ctstring &dialog) const;

    unsigned CalculateScriptSize() const;
    const list<pair<unsigned, ctstring> > GetScriptByPage() const;
};

#endif

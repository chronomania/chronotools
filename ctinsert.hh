#ifndef bqtCTinsertHH
#define bqtCTinsertHH

#include <list>
#include <string>
#include <vector>

#include "snescode.hh"
#include "wstring.hh"
#include "space.hh"
#include "fonts.hh"
#include "ctcset.hh"

#include "o65linker.hh"

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

    void LoadImages();
    void GenerateCode();
    void DictionaryCompress();

    unsigned GetFont12width(ctchar chronoch) const;
    
    void PatchROM(class ROM &ROM);

    void ReportFreeSpace();
    void ReorganizeFonts();
    
    insertor();
    ~insertor();

    // Shouldn't be public, but StringReceipt is an outside class that needs them
    void PlaceData(const vector<unsigned char>&, unsigned address, const string& reason="");
    void PlaceByte(unsigned char byte, unsigned address, const string& reason="");
    
private:
    struct stringdata
    {
        ctstring str;
        typedef enum { zptr8, zptr12, fixed, item, tech, monster } strtype;
        strtype type;
        unsigned width; // used if type==fixed;
        unsigned address;
    };
    
    typedef list<stringdata> stringlist;
    // strings: used in:
    //     LoadFile()
    //     GetZStringPageList()
    //     GetZStringList()
    //     WriteStrings()
    stringlist strings;
    
    list<SNEScode> codes;
    
    vector<ctstring> dict;
    
    Font8data Font8;
    Font8vdata Font8v;
    Font12data Font12;
    
    O65linker objects;

    freespacemap freespace;
    
    class Conjugatemap *Conjugater;
    
    void GenerateConjugatorCode();
    void GenerateCrononickCode();
    void GenerateVWF12code();
    void GenerateVWF8code();
    void GenerateSignatureCode();
    void WriteStrings();
    void WriteDictionary();
    void WriteRelocatedStrings();
    void WritePageZ(unsigned page, class stringoffsmap&);
    unsigned WriteStringTable(class stringoffsmap&, const string& what);
    void PatchTimeBoxes();
    
    /* Used to mark free space in the ROM because of code that is no longer used. */
    void ObsoleteCode(unsigned addr, unsigned bytes, bool barrier=false);
    
    void ApplyDictionary();
    void RebuildDictionary();
    
    const ctstring ParseScriptEntry(const ucs4string &input, const stringdata &model);
    const ctstring WrapDialogLines(const ctstring &dialog) const;

    // Get list of pages having zstrings
    const set<unsigned> GetZStringPageList() const;
    
    // Get zstrings of page
    const class stringoffsmap GetZStringList(unsigned pagenum) const;
    const class stringoffsmap GetStringList(stringdata::strtype type) const;

    unsigned CalculateScriptSize() const;

    /**
     *  LinkCalls - Parse add_call_of -commands from config file
     *     @section: The configuration file section
     *  Return value:
     *     false = There were no add_call_of -lines
     *     true  = ok
     */
    bool LinkCalls(const string& section);

    void LinkAndLocateCode();
    void AddReference(const ReferMethod& reference, unsigned target, const string& what="");
};

#endif

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
    void LoadAlreadyCompressedImage(const string& fn, unsigned address);
    void LoadAndCompressImage(const string& fn, unsigned address, unsigned char seg);
    
    void LoadAndCompressImageWithPointer
       (const string& fn, unsigned address, unsigned char seg);

    void LoadImages();
    void GenerateCode();
    void DictionaryCompress();

    unsigned GetFont12width(ctchar chronoch) const;
    
    void PatchROM(class ROM &ROM);

    void ReportFreeSpace();
    void ReorganizeFonts();
    
    insertor();
    ~insertor();
    
private:
    struct stringdata
    {
        ctstring str;
        enum { zptr8, zptr12, fixed } type;
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
    
    void WriteDictionary(class ROM &ROM);
    void WriteStrings(class ROM &ROM);
    void WriteCode(class ROM &ROM) const;

    void GenerateConjugatorCode();
    void GenerateCrononickCode();
    void GenerateVWF12code();
    void GenerateVWF8code();
    void GenerateSignatureCode();
    void PatchTimeBoxes();
    
    void PlaceData(const vector<unsigned char>&, unsigned address);
    void PlaceByte(unsigned char byte, unsigned address);
    
    void ApplyDictionary();
    void RebuildDictionary();
    
    const ctstring ParseScriptEntry(const ucs4string &input, const stringdata &model);
    const ctstring WrapDialogLines(const ctstring &dialog) const;

    // Get list of pages having zstrings
    const set<unsigned> GetZStringPageList() const;
    // Get zstrings of page
    const class stringoffsmap GetZStringList(unsigned pagenum) const;

    unsigned CalculateScriptSize() const;

#if 0    
    void LinkAndLocate(class FunctionList& functions);
#endif
    
    /**
     *  LinkCalls - Parse add_call_of -commands from config file
     *     @section: The configuration file section
     *     @code: The code to be linked
     */
    void LinkCalls(const string& section, const class O65& code);
};

#endif

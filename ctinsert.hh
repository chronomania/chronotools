#ifndef bqtCTinsertHH
#define bqtCTinsertHH

#include <list>
#include <string>
#include <vector>

#include "wstring.hh"
#include "space.hh"
#include "fonts.hh"
#include "snescode.hh"
#include "ctcset.hh"

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
    struct imagedata
    {
        unsigned address;
        vector<unsigned char> data;
    };
    typedef vector<imagedata> imagelist;
    imagelist images;
    
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

    freespacemap freespace;
    
    class Conjugatemap *Conjugater;
    
    void WriteDictionary(class ROM &ROM);
    void WriteStrings(class ROM &ROM);
    void Write8pixfont(class ROM &ROM) const;
    void Write8vpixfont(class ROM &ROM);
    void Write12pixfont(class ROM &ROM);
    void WriteCode(class ROM &ROM);
    void WriteImages(class ROM &ROM) const;

    void GenerateConjugatorCode();
    void GenerateVWF8code(unsigned widthtab_addr, unsigned tiletab_addr);
    void GenerateMogCode();
    
    void ApplyDictionary();
    void RebuildDictionary();
    
    const ctstring ParseScriptEntry(const ucs4string &input, const stringdata &model);
    const ctstring WrapDialogLines(const ctstring &dialog) const;
    
    // Get list of pages having zstrings
    const set<unsigned> GetZStringPageList() const;
    // Get zstrings of page
    const class stringoffsmap GetZStringList(unsigned pagenum) const;

    unsigned CalculateScriptSize() const;
    
    void LinkAndLocate(class FunctionList& functions);
};

#endif

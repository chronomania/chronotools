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

const std::string DispString(const ctstring &s, unsigned symbols_type=16);

class insertor
{
public:
    void LoadFile(FILE *fp);
    void LoadFont8(const string &fn) { Font8.Load(fn); }
    void LoadFont8v(const string &fn) { Font8v.Load(fn); }
    void LoadFont12(const string &fn) { Font12.Load(fn); }

    void ExpandROM();

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
        typedef enum
        {
            zptr8, zptr12, fixed,
            item, tech, monster,
            compressed,
            locationevent
        } strtype;
        strtype type;
        unsigned width; // used if type==fixed;
        unsigned address;

        /* ref_id is an index to ctinsert->refers.
         * Used when the entry is referred from various places.
         */
        unsigned ref_id; // used if zptr8 or zptr12

        /* tab_id is used in relocated dialog string tables.
         * It tells which *Z group this entry belongs to.
         */
        unsigned tab_id; // used if zptr8 or zptr12
    public:
        stringdata(): str(), type(), width(), address(), ref_id(), tab_id() { }
    };

    typedef list<stringdata> stringlist;
    stringlist strings;

    /* For relocated (by pointer edition) strings */
    typedef vector<list<ReferMethod> > referlist;
    referlist refers;

    /* For relocated (by table relocation) strings */
    unsigned table_counter;

    vector<ctstring> dict;

    Font8data Font8;
    Font8vdata Font8v;
    Font12data Font12;

    void PlaceData(const vector<unsigned char>&,
        unsigned address,
        const std::wstring& reason = L"");
    void PlaceByte(unsigned char byte,
        unsigned address,
        const std::wstring& reason = L"");

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
    void WriteOtherStrings(); //relocated item,monster,tech.
    void WriteCompressedStrings();
    void WriteRelocatedStrings();
    void WriteStringTable(stringdata::strtype type,
                          const string& tablename,
                          const string& what);

    /* Will mark free space in the ROM because of code that is no longer used.
     * addr and bytes are self-explanatory, but "barrier" tells whether
     * the block is still reachable. If barrier=true, the function
     * will generate a BRA over the block. If the block is too small,
     * it will be filled with NOP and nothing is freed.
     */
    void ObsoleteCode(unsigned addr, unsigned bytes, bool barrier=false);

    void ApplyDictionary();
    void RebuildDictionary();

    const ctstring ParseScriptEntry(const std::wstring &input, const stringdata &model);
    const ctstring WrapDialogLines(const ctstring &dialog) const;

    unsigned CalculateScriptSize() const;
    const list<pair<unsigned, ctstring> > GetScriptByPage() const;
private:
    insertor(const insertor&);
    const insertor&operator=(const insertor&);
};

#endif

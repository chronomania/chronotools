#ifndef bqtCTfontsHH
#define bqtCTfontsHH

#include <vector>

using std::vector;

#include "ctcset.hh"
#include "hash.hh"

typedef hash_map<ctchar, ctchar> Rearrangemap_t;

class Font12data
{
    vector<unsigned char> widths;
    vector<unsigned char> tiletab1;
    vector<unsigned char> tiletab2;
    
    unsigned charcount;
    
    string fn;
    
    void LoadBoxAs(unsigned boxno, unsigned tileno, class TGAimage &);
    
public:
    void Load(const string &filename);
    
    // Returns a table insertable to ROM.
    inline const vector<unsigned char>& GetTab1() const { return tiletab1; }
    inline const vector<unsigned char>& GetTab2() const { return tiletab2; }

    inline unsigned GetWidth(ctchar CharNum) const { return widths.at(CharNum); }
    
    unsigned GetCount() const;
    
    void Reload(const Rearrangemap_t& );
};

class Font8data
{
    vector<unsigned char> tiletable;
    vector<unsigned char> widths;
    
    string fn;

    void LoadBoxAs(unsigned boxno, unsigned tileno, class TGAimage &);
    
    virtual unsigned GetBegin() const;
    virtual unsigned GetCount() const;
    
public:
    virtual ~Font8data() { }
    
    void Load(const string &filename);
    
    // Returns a table insertable to ROM.
    inline const vector<unsigned char> &GetTiles() const { return tiletable; }
    inline unsigned GetWidth(unsigned char CharNum) const { return widths.at(CharNum); }
    inline const vector<unsigned char> &GetWidths() const { return widths; }
    
    void Reload(const Rearrangemap_t& );
};

class Font8vdata: public Font8data
{
    virtual unsigned GetBegin() const;
    virtual unsigned GetCount() const;
    
public:
    virtual ~Font8vdata() { }
};

#endif

#ifndef bqtCTfontsHH
#define bqtCTfontsHH

#include <vector>

#include "ctcset.hh"
#include "hash.hh"

typedef hash_map<ctchar, ctchar> Rearrangemap_t;

class Font12data
{
    std::vector<unsigned char> widths;
    std::vector<unsigned char> tiletab1;
    std::vector<unsigned char> tiletab2;

    unsigned charcount;

    std::string fn;

    void LoadBoxAs(unsigned boxno, unsigned tileno, class TGAimage &);

public:
    Font12data(): widths(), tiletab1(), tiletab2(), charcount(), fn() { }

    void Load(const std::string &filename);

    // Returns a table insertable to ROM.
    inline const std::vector<unsigned char>& GetTab1() const { return tiletab1; }
    inline const std::vector<unsigned char>& GetTab2() const { return tiletab2; }

    inline unsigned GetWidth(ctchar CharNum) const { return widths.at(CharNum); }

    unsigned GetCount() const;

    void Reload(const Rearrangemap_t& );
};

class Font8data
{
    std::vector<unsigned char> tiletable;
    std::vector<unsigned char> widths;

    std::string fn;

    void LoadBoxAs(unsigned boxno, unsigned tileno, class TGAimage &);

    virtual unsigned GetBegin() const;
    virtual unsigned GetCount() const;

public:
    Font8data();
    virtual ~Font8data();

    void Load(const std::string &filename);

    // Returns a table insertable to ROM.
    inline const std::vector<unsigned char> &GetTiles() const { return tiletable; }
    inline unsigned GetWidth(unsigned char CharNum) const { return widths.at(CharNum); }
    inline const std::vector<unsigned char> &GetWidths() const { return widths; }

    void Reload(const Rearrangemap_t& );
};

class Font8vdata: public Font8data
{
    virtual unsigned GetBegin() const;
    virtual unsigned GetCount() const;

public:
    virtual ~Font8vdata();
};

#endif

#ifndef bqtCTfontsHH
#define bqtCTfontsHH

#include <vector>
#include <string>

using std::vector;
using std::string;

class Font12data
{
    vector<unsigned char> widths;
    vector<unsigned char> tiletab1;
    vector<unsigned char> tiletab2;
public:
    void Load(const string &filename);
    
    // Returns a table insertable to ROM.
    inline unsigned GetWidth(unsigned char CharNum) const { return widths.at(CharNum); }
    inline const vector<unsigned char> &GetTab1() const { return tiletab1; }
    inline const vector<unsigned char> &GetTab2() const { return tiletab2; }
};

class Font8data
{
    vector<unsigned char> tiletable;
public:
    void Load(const string &filename);
    
    // Returns a table insertable to ROM.
    inline unsigned GetCharCount() const { return 256; }
    inline const vector<unsigned char> &GetTiles() const { return tiletable; }
};

#endif

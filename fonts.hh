#include <vector>
#include <string>

using std::vector;
using std::string;

class Font12data
{
    vector<unsigned char> tiletable;
    vector<unsigned char> widths;
public:
    void Load(const string &filename);
    
    // Returns a table insertable to ROM.
    inline unsigned GetCharCount() const { return 96; }
    inline const vector<unsigned char> &GetTiles() const { return tiletable; }
    inline unsigned GetWidth(unsigned char CharNum) const { return widths.at(CharNum); }
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

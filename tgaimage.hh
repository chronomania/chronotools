#include <vector>
#include <string>

using namespace std;

class TGAimage
{
public:
    enum palettetype
    {
        pal_6color,
        pal_4color,
        pal_16color
    };

    TGAimage(const string &filename);
    TGAimage(unsigned x, unsigned y, unsigned char color);
    
    void setboxsize(unsigned x, unsigned y) { xsize=x; ysize=y; }
    void setboxperline(unsigned n) { xbox = n; }
    
    const vector<char> getbox(unsigned boxnum) const;
    unsigned getboxcount() const;
    
    void Save(const string &fn, palettetype paltype, const unsigned *palette = NULL);
    void PSet(unsigned x,unsigned y, unsigned char value);
    unsigned char Point(unsigned x,unsigned y) const;
    
    unsigned GetXdim() const { return xdim; }
    unsigned GetYdim() const { return ydim; }
    unsigned GetPalSize() const { return palsize; }
    
private:
    unsigned xdim, ydim;
    unsigned palsize;
    vector<char> data;
    
    unsigned xsize, ysize; // size of each box
    unsigned xbox; // boxes per line
};

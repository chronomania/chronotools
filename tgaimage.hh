#include <vector>
#include <string>

class TGAimage
{
public:
    enum palettetype
    {
        pal_6color,
        pal_4color,
        pal_16color,
        pal_256color,
        pal_none
    };

    TGAimage(const std::string &filename);
    TGAimage(unsigned x, unsigned y, unsigned char color);
    
    void setboxsize(unsigned x, unsigned y) { xsize=x; ysize=y; }
    void setboxperline(unsigned n) { xbox = n; }
    
    const std::vector<unsigned char> getbox(unsigned boxnum) const;
    unsigned getboxcount() const;
    
    void Save(const std::string &fn, palettetype paltype=pal_none, const unsigned *palette = NULL);
    void PSet(unsigned x,unsigned y, unsigned value);
    unsigned Point(unsigned x,unsigned y) const;
    
    unsigned GetXdim() const { return xdim; }
    unsigned GetYdim() const { return ydim; }
    unsigned GetPalSize() const { return palsize; }
    unsigned GetPalEntry(unsigned n) const { return palette_in.at(n); }
    unsigned GetPixBitness() const { return pixbitness; }
    
    bool Error() const { return !GetXdim(); }
    
private:
    unsigned xdim, ydim;
    unsigned palsize, pixbitness;
    std::vector<unsigned char> data;
    std::vector<unsigned short> palette_in;
    
    unsigned xsize, ysize; // size of each box
    unsigned xbox; // boxes per line
};

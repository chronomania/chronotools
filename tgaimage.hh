#include <vector>
#include <string>

using namespace std;

class TGAimage
{
    unsigned xdim, ydim;
    vector<char> data;
    
    unsigned xsize, ysize; // size of each box
    unsigned xbox; // boxes per line
public:
    TGAimage(const string &filename);
    TGAimage(unsigned x, unsigned y, unsigned char color);
    
    void setboxsize(unsigned x, unsigned y) { xsize=x; ysize=y; }
    void setboxperline(unsigned n) { xbox = n; }
    
    const vector<char> getbox(unsigned boxnum) const;
    unsigned getboxcount() const;
};


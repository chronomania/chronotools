#ifndef bqtCTromHH
#define bqtCTromHH

#include <vector>

#include "snescode.hh"
#include "refer.hh"

using namespace std;

class ROM
{
    vector<unsigned char> Data;
    vector<unsigned char> Touched;


public:
    ROM(unsigned size) : Data(size, 0), Touched(size, false)
    {
    }
    inline void Write(unsigned offs, unsigned char value)
    {
        Data[offs]    = value;
        Touched[offs] = true;
    }
    const unsigned char &operator[] (unsigned ind) const { return Data[ind]; }
    const unsigned size() const { return Data.size(); }
    bool touched(unsigned ind) const { return Touched[ind]; }
    
    void AddReference(const ReferMethod& reference,
                      unsigned target,
                      const string& what = "");

    void AddPatch(const SNEScode& code);

    // Writes a subroutine. Remember to terminate it with RTL if necessary.
    void AddPatch(const vector<unsigned char>& code, unsigned addr, const string& what="");
};

#endif

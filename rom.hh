#ifndef bqtCTromHH
#define bqtCTromHH

#include <vector>

#include "snescode.hh"

using namespace std;

class ROM
{
    vector<unsigned char> Data;
    vector<unsigned char> Touched;

    // Adds JSL to the specific location.
    void AddCall(unsigned codeaddress, unsigned target);
    
    // Writes a subroutine. Remember to terminate it with LONG-RETURN!
    void AddSubRoutine(unsigned target, const vector<unsigned char> &code);
    
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
    
    void AddCall(unsigned address, const SNEScode &code);
};

#endif

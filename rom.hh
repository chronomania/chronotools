#ifndef bqtCTromHH
#define bqtCTromHH

#include <vector>
#include <string>

#include "dataarea.hh"

class ROM
{
    DataArea Data;
    unsigned length;

public:
    ROM(unsigned siz): length(siz)
    {
    }
    void Write(unsigned pos, unsigned char value);
    
    const unsigned char operator[] (unsigned pos) const { return Data.GetByte(pos); }
    const unsigned size() const { return length; }

    unsigned FindNextBlob(unsigned where, unsigned& length) const;
    
    const std::vector<unsigned char> GetContent() const;
    
    // FIXME: This function is not 100% certainly working correctly
    const std::vector<unsigned char> GetContent(unsigned begin, unsigned size) const;

    // Writes a subroutine. Remember to terminate it with RTL if necessary.
    void AddPatch(const std::vector<unsigned char>& code,
                  unsigned addr,
                  const std::string& what="");
};

#endif

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
    
    const unsigned size() const { return length; }

    unsigned FindNextBlob(unsigned where, unsigned& length) const;
    
    const std::vector<unsigned char> GetContent() const;
    
    const std::vector<unsigned char> GetContent(unsigned begin, unsigned size) const;

    void AddPatch(const std::vector<unsigned char>& code,
                  unsigned addr,
                  const std::string& what="");

    void SetZero(unsigned begin, unsigned size, const std::string& why="");

    /* These don't need to be private, but they are
     * now to ensure Chronotools doesn't use them.
     */
private:
    void Write(unsigned pos, unsigned char value);
    
    const unsigned char operator[] (unsigned pos) const { return Data.GetByte(pos); }

};

#endif

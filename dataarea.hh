#ifndef bqt65asmDataAreaHH
#define bqt65asmDataAreaHH

#include <map>
#include <vector>

class DataArea
{
    typedef std::vector<unsigned char> vec;
    typedef std::map<unsigned, vec> map;
    map blobs;
private:
    map::iterator GetRef(unsigned offset);
    static void PrepareVec(vec&, unsigned offset);
    
    void Optimize(map::iterator i);
public:
    void WriteByte(unsigned pos, unsigned char byte);
    unsigned char GetByte(unsigned pos) const;
    
    unsigned GetBase() const;
    unsigned GetTop() const;
    unsigned GetSize() const { return GetTop() - GetBase(); }

    unsigned FindNextBlob(unsigned where, unsigned& length) const;

    const std::vector<unsigned char> GetContent() const;
    
    // FIXME: This function is not 100% certainly working correctly
    const std::vector<unsigned char> GetContent(unsigned begin, unsigned size) const;
};

#endif

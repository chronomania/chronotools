/*** CRC32 calculation (CRC::update) ***/
namespace
{
    class CRC
    {
        unsigned long crctable[256];
    public:
        CRC()
        {
            unsigned long poly = 0xEDB88320L;
            for(unsigned i=0; i<256; ++i)
            {
                unsigned long crc = i;
                for(unsigned j=8; j-->0; ) { bool c=crc&1; crc>>=1; if(c)crc^=poly; }
                crctable[i] = crc;
            }
        }
        inline unsigned long update(unsigned long crc, unsigned char b) const
        {
            return ((crc >> 8) & 0x00FFFFFF) ^ crctable[(crc^b) & 0xFF];
        }
    } CRC32;
}

#include "crc32.h"
crc32_t crc32_update(crc32_t c, unsigned char b)
{
    return CRC32.update(c, b);
}

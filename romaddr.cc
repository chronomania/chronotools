#include "romaddr.hh"

unsigned char ROM2SNESpage(unsigned char page)
{
    if(page < 0x40) return page | 0xC0;
    
    /* Pages 00..3F have only their high part mirrored */
    /* Pages 7E..7F can not be used (they're RAM) */
    
    if(page >= 0x7E) return page & 0x3F;
    return (page - 0x40) + 0x40;
}

unsigned char SNES2ROMpage(unsigned char page)
{
    if(page >= 0x80) return page & 0x3F;
    return (page & 0x3F) + 0x40;
}

unsigned long ROM2SNESaddr(unsigned long addr)
{
    return (addr & 0xFFFF) | (ROM2SNESpage(addr >> 16) << 16);
}

unsigned long SNES2ROMaddr(unsigned long addr)
{
    return (addr & 0xFFFF) | (SNES2ROMpage(addr >> 16) << 16);
}

bool IsSNESbased(unsigned long addr)
{
    return addr >= 0xC00000;
}

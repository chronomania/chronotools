#ifndef bqtReferHH
#define bqtReferHH

struct ReferMethod
{
    unsigned from_addr;
    unsigned long or_mask;
    unsigned char num_bytes;
    signed char shr_by;
protected:
    ReferMethod(unsigned long o,
                unsigned char n,
                signed char s,
                unsigned from)
      : from_addr(from),
        or_mask(o), num_bytes(n), shr_by(s) { }
};
struct CallFrom: public ReferMethod
{
    // JSL to specific location.
    CallFrom(unsigned from): ReferMethod(0xC0000022,4,-8, from) {}
    // Formula: (value << 8) | 0xC0000022, 4 bytes
};
struct LongPtrFrom: public ReferMethod
{
    LongPtrFrom(unsigned from): ReferMethod(0xC00000,3,0, from) {}
    // Formula: (value | 0xC00000), 3 bytes
};
struct OffsPtrFrom: public ReferMethod
{
    OffsPtrFrom(unsigned from): ReferMethod(0,2,0, from) {}
    // Formula: (value), 2 bytes
};
struct PagePtrFrom: public ReferMethod
{
    PagePtrFrom(unsigned from): ReferMethod(0xC0,1,16, from) {}
    // Formula: (value >> 16) | 0xC0, 1 byte
};

#endif

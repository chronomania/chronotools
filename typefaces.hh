#ifndef bqtCTtypefacesHH
#define bqtCTtypefacesHH

#include <vector>

using std::vector;

#include "wstring.hh"

class Typeface
{
    ucs4string begin_marker;
    ucs4string end_marker;
    unsigned offset;
    unsigned begin, end;
    unsigned condense;
public:
    Typeface() : begin_marker(), end_marker(),
                 offset(), begin(), end(), condense() { }

    Typeface(const ucs4string&b, const ucs4string &e,
             unsigned o,unsigned B,unsigned E,unsigned C)
     : begin_marker(b), end_marker(e),
       offset(o), begin(B), end(E), condense(C)
     { }

    const ucs4string& get_begin_marker() const { return begin_marker; }
    const ucs4string& get_end_marker() const { return end_marker; }
    unsigned get_offset() const { return offset; }
    unsigned get_begin() const { return begin; }
    unsigned get_end() const { return end; }
    unsigned get_condense() const { return condense; }
};

extern vector<Typeface> Typefaces;
extern void LoadTypefaces();

#endif

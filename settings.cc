#include "settings.hh"

namespace
{
    const unsigned Constants[][2] =
    {
        {0x3F8C60, 0x3F8C60},    // 8pix font tile table location

        {0x0258BB, 0x0258C4},    // Byte: Font starts here, dictionary ends here

        {0x025E29, 0x000000},    // Word: The offset of width12 table
        {0x025E2B, 0x000000},    // Byte: The segment of width12 table
        {0x025E25, 0x000000},    // Byte: Index of the first byte in the width table

        {0x025DD2, 0x025D96},    // Word: The offset of VWF12 first tiletable
        {0x025DE3, 0x025DA7},    // Word: The offset of VWF12 second tiletable
        {0x025DFD, 0x025DC1},    // Byte: The segment of VWF12 tiletables

        {0x0258DE, 0x0258E7},    // Word: Dictionary table offset
        {0x0258E0, 0x0258E9},    // Byte: Dictionary table segment
        {0x0258E9, 0x0258F2},    // Byte: Dictionary table segment (duplicate)
        {1,        0       },    // Bool: Use width-table

        {0x025DC4, 0x025D88},    // Where is the function that draws text
        {0x030557, 0       },    // Where is the image decompressor?
        {0x3CF9F0, 0       }     // Where is the location event pointer table?
    };

    unsigned Lang=0;
}

void SelectENGconst() { Lang=0; }
void SelectJAPconst() { Lang=1; }

unsigned GetConst(unsigned which)
{
    return Constants[which][Lang];
}

// writeout.cc:
namespace
{
    // Where will the conjugater call go
    const unsigned ConjugatePatchAddress = 0x0258C2;
    
    // The rest of the address list is in ctdump.cc
}

enum
{
    TILETAB_8_ADDRESS=0,
    CSET_BEGINBYTE,
    VWF12_WIDTH_OFFSET,
    VWF12_WIDTH_SEGMENT,
    VWF12_WIDTH_INDEX,    
    VWF12_TAB1_OFFSET,
    VWF12_TAB2_OFFSET,
    VWF12_SEGMENT,
    DICT_OFFSET,
    DICT_SEGMENT1,
    DICT_SEGMENT2,
    VWF12_WIDTH_USE,
    DIALOG_DRAW_FUNC_ADDR,
    DECOMPRESSOR_FUNC_ADDR
};

unsigned GetConst(unsigned which);
void SelectENGconst();
void SelectJAPconst();

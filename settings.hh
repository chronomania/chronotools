// writeout.cc:
namespace
{
    const unsigned Font8_Address         = 0x3F8C60;
    const unsigned ConjugatePatchAddress = 0x0258C2;
    const unsigned FirstChar_Address     = 0x0258BB;
    
    const unsigned WidthTab_Address_Ofs  = 0x025E29;
    const unsigned WidthTab_Address_Seg  = 0x025E2B;
    const unsigned WidthTab_Offset_Addr  = 0x025E25;

    const unsigned Font12a_Address_Ofs  = 0x025DD2;
    const unsigned Font12b_Address_Ofs  = 0x025DE3;
    const unsigned Font12_Address_Seg   = 0x025DFD; // note: same segment
    
    const unsigned DictAddr_Ofs         = 0x0258DE;
    const unsigned DictAddr_Seg_1       = 0x0258E0;
    const unsigned DictAddr_Seg_2       = 0x0258E9;
    
    // The rest of the address list is in ctdump.cc
}

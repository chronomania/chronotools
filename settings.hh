// writeout.cc:
namespace
{
    // Where the 8pix font tile table is
    const unsigned Font8_Address         = 0x3F8C60;
    
    // Where will the conjugater call go
    const unsigned ConjugatePatchAddress = 0x0258C2;
    
    // Where is the function that draws text
    const unsigned DialogDrawFunctionAddr = 0x025DC4;
    
    // Address of the byte that normally is 0xA0
    const unsigned FirstChar_Address     = 0x0258BB;
    
    // Address of the 12pix font width table pointer
    const unsigned WidthTab_Address_Ofs  = 0x025E29;
    const unsigned WidthTab_Address_Seg  = 0x025E2B;
    
    // Address of the byte that will be added
    // to widthtab address when using
    const unsigned WidthTab_Offset_Addr  = 0x025E25;

    // Addresses of the pointers to 12pix font
    const unsigned Font12a_Address_Ofs  = 0x025DD2;
    const unsigned Font12b_Address_Ofs  = 0x025DE3;
    const unsigned Font12_Address_Seg   = 0x025DFD; // note: same segment
    
    // Address of the pointer to dictionary pointer table
    const unsigned DictAddr_Ofs         = 0x0258DE;
    // (The segment part is in two addresses)
    const unsigned DictAddr_Seg_1       = 0x0258E0;
    const unsigned DictAddr_Seg_2       = 0x0258E9;
    
    // The rest of the address list is in ctdump.cc
}

#include "typefaces.hh"
#include "ctcset.hh"
#include "config.hh"

vector<Typeface> Typefaces;

void LoadTypefaces()
{
    const ConfParser::ElemVec& elems = GetConf("font", "typeface").Fields();
    
    for(unsigned a=0; a<elems.size(); a += 6)
    {
        const ucs4string& begin_marker = elems[a];
        const ucs4string& end_marker   = elems[a+1];
        unsigned offset = elems[a+2];
        unsigned begin  = elems[a+3];
        unsigned end    = elems[a+4];
        unsigned condense=elems[a+5];
        
        bool empty = true;
        for(unsigned c=begin; c<end; ++c)
            if(getucs4(c, cset_12pix) != ilseq)
            {
                empty = false;
                break;
            }
        
        if(!empty)
        {
            fprintf(stderr,
                "Warning: Range 0x%X..0x%X allocated for a typeface overrides\n"
                "         some characters in the character set!\n",
                    begin, end);
        }
        
        Typefaces.push_back(Typeface(begin_marker, end_marker, offset, begin, end, condense));
    }
}

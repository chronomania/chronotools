#include <cstdio>

#include "ctcset.hh"
#include "ctinsert.hh"
#include "logfiles.hh"
#include "symbols.hh"
#include "config.hh"
#include "conjugate.hh"
#include "msginsert.hh"

const ctstring insertor::WrapDialogLines(const ctstring &dialog) const
{
    const ctstring &input = dialog;
    
    // Standard defines that this'll be initialized upon the first call.
    static unsigned MaxTextWidth   = GetConf("wrap", "maxtextwidth");
    static const bool verify_wraps = GetConf("wrap", "verify_wraps");
    
    FILE *log = GetLogFile("wrap", "log_wraps");
    
    unsigned row=0, col=0;
    
    ctstring result;
    
    static const unsigned char spacechar = getctchar(' ', cset_12pix);
    static const unsigned char colonchar = getctchar(':', cset_12pix);
    static const unsigned char eightchar = getctchar('8', cset_12pix);
    static const unsigned char dblw_char = getctchar('W', cset_12pix);

    unsigned space3width = (GetFont12width( spacechar ) ) * 3;
    unsigned num8width   = (GetFont12width( eightchar ) );
    unsigned w5width     = (GetFont12width( dblw_char ) ) * 5;
    
    const unsigned fontbegin = get_font_begin();
    //const unsigned dictend   = fontbegin;

    unsigned wrappos = 0;
    unsigned wrapcol = 0;
    
    bool wraps_happened = false;
    bool wrap_with_indent = false;
    
    bool linelength_error = false;
    bool linecount_error = false;
    
    for(unsigned a=0; a<input.size(); ++a)
    {
        ctchar c = input[a];
        
        if(c == spacechar)
        {
            // MessageWorking(); - too much slowdown
            
            if(!col) wrap_with_indent = true;
            wrappos = result.size();
            wrapcol = col;
        }
        if(c == colonchar)
            wrap_with_indent = true;

        result += c;
        switch(c)
        {
            case 0x05: // nl
                ++row;
                col = 0;
                wrap_with_indent = false;
                break;
            case 0x06: // nl3
                ++row;
                col = space3width;
                wrap_with_indent = true;
                break;
            case 0x0B: // pause
            case 0x09: // cls
                row = 0;
                col = 0;
                wrap_with_indent = false;
                break;
            case 0x0C: // pause3
            case 0x0A: // cls3
                row = 0;
                col = space3width;
                wrap_with_indent = true;
                break;
            case 0x0D: // num8  -- assume 88
                col += num8width * 2;
                break;
            case 0x0E: // num16 -- assume 88888
                col += num8width * 5;
                break;
            case 0x0F: // num32 -- assume 8888888888
                col += num8width * 10;
                break;
            case 0x1F: // item -- assume "" - FIXME
            {
                col += 1;
                break;
            }
            case 0x11: // User definable names
            case 0x13: // member, Crono
            case 0x14: // Marle
            case 0x15: // Lucca
            case 0x16: // Robo
            case 0x17: // Frog
            case 0x18: // Ayla
            case 0x19: // Magus
            case 0x1A: // crononick
            case 0x1B: // member1, member2, member3
            case 0x1C: // and Epoch
            case 0x1D: // Assume all of them are of maximal
            case 0x20: // length, that is 'WWWWW'.
            {
                col += w5width;
                break;
            }
            case 0x1E: // Nadia:
            {
                static const string nadia = WstrToAsc(Symbols.GetRev(16).find(0x1E)->second);
                // This name can't be changed by the player
                for(unsigned a=0; a<nadia.size(); ++a)
                {
                    ctchar c = getctchar(nadia[a], cset_12pix);
                    col += GetFont12width(c);
                }
                break;
            }
            default:
            {
                if(Conjugater->IsConjChar(c))
                {
                    col += Conjugater->GetMaxWidth(c);
                }
                else if(c >= fontbegin)
                {
                    col += GetFont12width(c);
                }
                break;
            }
        }
        if(col >= MaxTextWidth)
        {
            /*
            fprintf(stderr, "\nWrap (col=%3u,row=%3u), result=%s\n",
                col,row,DispString(result).c_str());
            */
            wraps_happened = true;
            
            col -= wrapcol;
            if(wrap_with_indent)
            {
                result[wrappos] = 0x06; // nl3
                col += space3width;
            }
            else
            {
                result[wrappos] = 0x05; // nl
            }
            wrappos = result.size();
            wrapcol = col;
            ++row;
            /*
            fprintf(stderr,  "Now  (col=%3u,row=%3u), result=%s\n",
                col,row,DispString(result).c_str());
            */
        }
        if(row >= 4) linecount_error = true;
        if(col >= MaxTextWidth) linelength_error = true;
    }
    
    if(linecount_error || linelength_error)
    {
        MessageTooLongText(input, result);
    }
    
    if(wraps_happened)
    {
        if(log)
        {
            fprintf(log,
              "Warning: Done some wrapping\n"
              "In:  %s\n"
              "Out: %s\n"
              "\n",
                  DispString(input).c_str(),
                  DispString(result).c_str()
             );
        }
        if(verify_wraps)
        {
            result = WrapDialogLines(result);
        }
    }
    
#if 0
    if(log)
    {
        fprintf(log, "Wrap('%s')\n"
                     "->   '%s'\n",
            DispString(input).c_str(),
            DispString(result).c_str());
    }
#endif

    return result;
}

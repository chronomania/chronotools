#include "rom.hh"

#define DEBUG_ADDING_SUB     0
#define DEBUG_ADDING_CALL    1
#define DEBUG_ADDING_LONGPTR 1
#define DEBUG_ADDING_OFFSPTR 1
#define DEBUG_ADDING_PAGEPTR 1

// Far call takes four bytes:
//     22 63 EA C0 = JSL $C0:$EA63

// Near jump takes three bytes:
//     4C 23 58    = JMP db:$5823    (db=register)

// Short jump takes two bytes:
//     80 2A       = JMP (IP + 2 + $2A)

void ROM::AddCall(unsigned codeaddress, unsigned target)
{
    unsigned rompos = codeaddress & 0x3FFFFF;
    
    target |= 0xC00000; // Ensure we're jumping correctly
    
#if DEBUG_ADDING_CALL
    fprintf(stderr, "- Adding subroutine    $%06X call at $%06X\n",
        target, 0xC00000 | rompos);
#endif

    Write(rompos++, 0x22);
    Write(rompos++, target & 255);
    Write(rompos++, (target >> 8) & 255);
    Write(rompos  , target >> 16);
}

void ROM::AddLongPtr(unsigned codeaddress, unsigned target)
{
    unsigned rompos = codeaddress & 0x3FFFFF;
    
    target |= 0xC00000; // Ensure we're pointing correctly
    
#if DEBUG_ADDING_LONGPTR
    fprintf(stderr, "-   Writing longptr    $%06X at $%06X\n", target, rompos);
#endif
    
    Write(rompos++, target & 255);
    Write(rompos++, (target >> 8) & 255);
    Write(rompos  , target >> 16);
}

void ROM::AddOffsPtr(unsigned codeaddress, unsigned target)
{
    unsigned rompos = codeaddress & 0x3FFFFF;
    
    target &= 0xFFFF;
    
#if DEBUG_ADDING_OFFSPTR
    fprintf(stderr, "-   Writing offsptr      $%04X at $%06X\n", target, rompos);
#endif
    
    Write(rompos++, target & 255);
    Write(rompos,   (target >> 8) & 255);
}

void ROM::AddPagePtr(unsigned codeaddress, unsigned target)
{
    unsigned rompos = codeaddress & 0x3FFFFF;
    
    target >>= 16;
    target |= 0xC0;
    
#if DEBUG_ADDING_PAGEPTR
    fprintf(stderr, "-   Writing pageptr    $%02X     at $%06X\n", target, rompos);
#endif
    
    Write(rompos, target & 255);
}

void ROM::AddSubRoutine(unsigned target, const vector<unsigned char> &code)
{
#if DEBUG_ADDING_SUB
    fprintf(stderr, "- Adding subroutine at $%06X (%u bytes)\n",
        0xC00000 | target,
        code.size());
#endif
    unsigned rompos = target & 0x3FFFFF;
    
    for(unsigned a=0; a<code.size(); ++a)
        Write(rompos++, code[a]);
}

void ROM::AddPatch(const SNEScode &code)
{
    AddSubRoutine(code.GetAddress(), code);
    
    if(true) /* Handle calls */
    {
        const set<unsigned>& addrlist = code.GetCalls();
        for(set<unsigned>::const_iterator
            i = addrlist.begin();
            i != addrlist.end();
            ++i)
        {
            AddCall(*i, code.GetAddress());
        }
    }

    if(true) /* Handle longptrs */
    {
        const set<unsigned>& addrlist = code.GetLongPtrs();
        for(set<unsigned>::const_iterator
            i = addrlist.begin();
            i != addrlist.end();
            ++i)
        {
            AddLongPtr(*i, code.GetAddress());
        }
    }

    if(true) /* Handle offsptrs */
    {
        const set<unsigned>& addrlist = code.GetOffsPtrs();
        for(set<unsigned>::const_iterator
            i = addrlist.begin();
            i != addrlist.end();
            ++i)
        {
            AddOffsPtr(*i, code.GetAddress());
        }
    }

    if(true) /* Handle pageptrs */
    {
        const set<unsigned>& addrlist = code.GetPagePtrs();
        for(set<unsigned>::const_iterator
            i = addrlist.begin();
            i != addrlist.end();
            ++i)
        {
            AddPagePtr(*i, code.GetAddress());
        }
    }
}

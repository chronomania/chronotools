#include "rom.hh"
#include "config.hh"
#include "logfiles.hh"

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
    
    FILE *log = GetLogFile("mem", "log_addrs");
    
    if(log)
        fprintf(log, "- Adding subroutine    $%06X call at $%06X\n",
            target, 0xC00000 | rompos);

    Write(rompos++, 0x22);
    Write(rompos++, target & 255);
    Write(rompos++, (target >> 8) & 255);
    Write(rompos  , target >> 16);
}

void ROM::AddLongPtr(unsigned codeaddress, unsigned target)
{
    unsigned rompos = codeaddress & 0x3FFFFF;
    
    target |= 0xC00000; // Ensure we're pointing correctly
    
    FILE *log = GetLogFile("mem", "log_addrs");
    
    if(log)
        fprintf(log, "-   Writing longptr    $%06X at $%06X\n", target, rompos);
    
    Write(rompos++, target & 255);
    Write(rompos++, (target >> 8) & 255);
    Write(rompos  , target >> 16);
}

void ROM::AddOffsPtr(unsigned codeaddress, unsigned target)
{
    unsigned rompos = codeaddress & 0x3FFFFF;
    
    target &= 0xFFFF;
    
    FILE *log = GetLogFile("mem", "log_addrs");
    
    if(log)
        fprintf(log, "-   Writing offsptr      $%04X at $%06X\n", target, rompos);
    
    Write(rompos++, target & 255);
    Write(rompos,   (target >> 8) & 255);
}

void ROM::AddPagePtr(unsigned codeaddress, unsigned target)
{
    unsigned rompos = codeaddress & 0x3FFFFF;
    
    target >>= 16;
    target |= 0xC0;
    
    FILE *log = GetLogFile("mem", "log_addrs");
    
    if(log)
        fprintf(log, "-   Writing pageptr    $%02X     at $%06X\n", target, rompos);
    
    Write(rompos, target & 255);
}

void ROM::AddSubRoutine(unsigned target, const vector<unsigned char> &code)
{
    if(code.empty()) return;
    
    FILE *log = GetLogFile("mem", "log_addrs");
    
    if(log)
        fprintf(log, "- Adding subroutine at $%06X (%u bytes)\n",
            0xC00000 | target,
            code.size());

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

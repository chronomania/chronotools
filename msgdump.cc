#include "msgdump.hh"
#include "wstring.hh"

#include <cstdio>

namespace
{
    bool MsgStateStrDump = false; // if dumping strings
}

void MessageBeginDumpingStrings(unsigned offs)
{
    std::fprintf(stderr, "Dumping strings at %06X...", offs);
    MsgStateStrDump = true;
}
void MessageBeginDumpingEvent(unsigned evno)
{
    std::fprintf(stderr, "Dumping event %03X...", evno);
    MsgStateStrDump = true;
}
void MessageBeginDumpingImage(const std::string& filename, const std::wstring& what)
{
    std::fprintf(stderr, "Creating %s (%s)...", filename.c_str(), WstrToAsc(what).c_str());
}
void MessageDone()
{
    std::fprintf(stderr, " done%s",
                 MsgStateStrDump ? "   \r" : "\n");
    MsgStateStrDump = false;
}
void MessageWorking()
{
}

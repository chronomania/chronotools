#include "msgdump.hh"
#include "wstring.hh"

namespace
{
    bool MsgStateStrDump = false; // if dumping strings
}

void MessageBeginDumpingStrings(unsigned offs)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    MsgStateStrDump = true;
}
void MessageBeginDumpingImage(const std::string& filename, const std::wstring& what)
{
    fprintf(stderr, "Creating %s (%s)...", filename.c_str(), WstrToAsc(what).c_str());
}
void MessageDone()
{
    fprintf(stderr, " done%s",
                    MsgStateStrDump ? "   \r" : "\n");
    MsgStateStrDump = false;
}
void MessageWorking()
{
}

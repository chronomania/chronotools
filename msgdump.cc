#include "msgdump.hh"

namespace
{
    bool MsgStateStrDump = false; // if dumping strings
}

void MessageBeginDumpingStrings(unsigned offs)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    MsgStateStrDump = true;
}
void MessageBeginDumpingImage(const string& filename, const string& what)
{
    fprintf(stderr, "Creating %s (%s)...", filename.c_str(), what.c_str());
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

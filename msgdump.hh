#include "wstring.hh"

void MessageBeginDumpingStrings(unsigned offs);
void MessageBeginDumpingEvent(unsigned evno);
void MessageBeginDumpingImage(const std::string& filename, const std::wstring& what);
void MessageDone();
void MessageWorking();

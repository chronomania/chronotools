#include <string>

using std::string;

void MessageBeginDumpingStrings(unsigned offs);
void MessageBeginDumpingImage(const string& filename, const string& what);
void MessageDone();
void MessageWorking();

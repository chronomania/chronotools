#include <string>

using std::string;

void OpenScriptFile(const string& filename);
void CloseScriptFile();

void PutAscii(const string& comment);

void BlockComment(const string& comment);
void StartBlock(const char* blocktype, const string& reason, unsigned intparam);
void StartBlock(const string& blocktype, const string& reason);
void PutBase62Label(const unsigned noffs);
void PutBase16Label(const unsigned noffs);
void PutBase10Label(const unsigned noffs);
void PutContent(const string& s, bool dolf = true);
void EndBlock();

const string Base62Label(const unsigned noffs);

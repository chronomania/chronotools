#include "wstring.hh"

void OpenScriptFile(const std::string& filename);
void CloseScriptFile();

void PutAscii(const std::wstring& comment);

void BlockComment(const std::wstring& comment);
void StartBlock(const std::wstring& blocktype, const std::wstring& reason, unsigned intparam);
void StartBlock(const std::wstring& blocktype, const std::wstring& reason);
void PutBase62Label(const unsigned noffs);
void PutBase16Label(const unsigned noffs);
void PutBase10Label(const unsigned noffs);
void PutContent(const std::wstring& s, bool dolf = true);
void EndBlock();

const std::wstring Base62Label(const unsigned noffs);

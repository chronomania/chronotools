#include <cstdio>
#include "config.hh"
#include "ctcset.hh"

using namespace std;

#include "base62.hh"
#include "wstring.hh"
#include "scriptfile.hh"

static FILE *scriptout;

void OpenScriptFile(const string& filename)
{
    scriptout = fopen(filename.c_str(), "wt");
    if(!scriptout)
    {
        perror(filename.c_str());
    }
}

void CloseScriptFile()
{
    fclose(scriptout);
}

void PutAscii(const std::wstring& comment)
{
    static wstringOut conv;
    conv.SetSet(getcharset());
    fprintf(scriptout, "%s", conv.puts(comment).c_str());
}

static std::wstring CurLabel = L"";
static std::wstring CurLabelComment = L"";

void BlockComment(const std::wstring& comment)
{
    CurLabelComment = comment;
}
void StartBlock(const std::wstring& blocktype,
                const std::wstring& reason,
                unsigned intparam)
{
    StartBlock(wformat(blocktype.c_str(), intparam), reason);
}
void StartBlock(const std::wstring& blocktype, const std::wstring& reason)
{
    bool newlabel = true;//CurLabel != Buf;
    bool comment = !CurLabelComment.empty();

    if(newlabel)
    {
        if(!CurLabel.empty()) PutAscii(L"\n\n");
    }

    if(newlabel)
    {
        // FIXME: iconv here
        if(!blocktype.empty())
        {
            std::wstring blockheader = L"*";
            blockheader += blocktype;
            blockheader += L';';
            blockheader += reason;
            blockheader += L'\n';
            PutAscii(blockheader);
        }
        CurLabel = blocktype;
    }
    if(comment)
    {
        PutAscii(L";-----------------\n");
        PutAscii(CurLabelComment);
        PutAscii(L";-----------------\n");
    }
}
void EndBlock()
{
    //PutAscii("\n\n");
    CurLabelComment.clear();
}
const std::wstring Base62Label(const unsigned noffs)
{
    return AscToWstr(EncodeBase62(noffs, 4));
}
void PutBase62Label(const unsigned noffs)
{
    PutAscii(wformat(L"$%ls:", Base62Label(noffs).c_str()));
}
void PutBase16Label(const unsigned noffs)
{
    PutAscii(wformat(L"$%X:", noffs));
}
void PutBase10Label(const unsigned noffs)
{
    PutAscii(wformat(L"$%u:", noffs));
}
void PutContent(const std::wstring& s, bool dolf)
{
    if(dolf) PutAscii(L"\n");
    PutAscii(s);
    PutAscii(L"\n");
}

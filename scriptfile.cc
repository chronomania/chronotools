#include <cstdio>
#include "config.hh"
#include "ctcset.hh"

using namespace std;

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

void PutAscii(const string& comment)
{
    wstringOut conv(getcharset());
    string line = conv.puts(AscToWstr(comment));
    fprintf(scriptout, "%s", line.c_str());
}

static string CurLabel = "", CurLabelComment = "";
void BlockComment(const string& comment)
{
    CurLabelComment = comment;
}
void StartBlock(const char* blocktype, const string& reason, unsigned intparam)
{
    char *Buf = new char[strlen(blocktype) + 64];
    sprintf(Buf, blocktype, intparam);
    
    bool newlabel = CurLabel != Buf;
    bool comment = !CurLabelComment.empty();
    
    if(newlabel)
    {
        if(!CurLabel.empty()) PutAscii("\n\n");
    }
    
    if(newlabel)
    {
        // FIXME: iconv here
        if(*Buf)
        {
            string blockheader = "*";
            blockheader += Buf;
            blockheader += ';';
            blockheader += reason;
            blockheader += '\n';
            PutAscii(blockheader);
        }
        CurLabel = Buf;
    }
    if(comment)
    {
        PutAscii(";-----------------\n");
        PutAscii(CurLabelComment.c_str());
        PutAscii(";-----------------\n");
    }
    
    delete[] Buf;
}
void EndBlock()
{
    //PutAscii("\n\n");
    CurLabelComment = "";
}
void PutBase62Label(const unsigned noffs)
{
    string line = "$";
    for(unsigned k=62*62*62; ; k/=62)
    {
        unsigned dig = (noffs/k)%62;
        if(dig < 10) line += ('0' + dig);
        else if(dig < 36) line += ('A' + (dig-10));
        else line += ('a' + (dig-36));
        if(k==1)break;
    }
    line += ':';
    PutAscii(line);
}
void PutBase16Label(const unsigned noffs)
{
    char Buf[64]; sprintf(Buf, "$%X:", noffs);
    PutAscii(Buf);
}
void PutBase10Label(const unsigned noffs)
{
    char Buf[64]; sprintf(Buf, "$%u:", noffs);
    PutAscii(Buf);
}
void PutContent(const string& s, bool dolf)
{
    // FIXME: iconv here
    if(dolf) PutAscii("\n");

    // No iconv here - already done.
    fprintf(scriptout, "%s\n", s.c_str());
}


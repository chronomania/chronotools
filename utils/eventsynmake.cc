#include <cstdio>
#include <iostream>
#include <sstream>
#include <map>
#include <list>
#include <vector>
#include <bitset>

#include "xml.hh"
#include "wstring.hh"

/* Maximum number of opcodes allowed.
 * For std::bitset.
 */
#define MAX_OPS 16384

static unsigned CurSeqNo;
static std::vector<std::wstring> CurSeq;

struct Cmd
{
    std::wstring cmd;
    std::wstring size;
};

static std::map<std::wstring, std::wstring> parammap;
static std::vector<Cmd> cmds;
static std::map<std::wstring, std::list<unsigned> > ops;

static void BeginParam(std::wstringstream& cmdbuf,
                       const std::wstring& /*pos*/, const std::wstring& /*name*/)
{
    /*
        add(data, pos, name)
        add(data, pos)
        add(data, name)
     */
    cmdbuf << "\t\tcmd.Add(";
}
static void EndParam(std::wstringstream& cmdbuf,
                     const std::wstring& pos, const std::wstring& name)
{
    if(!pos.empty()) cmdbuf << ", " << pos << "U";
    if(!name.empty()) cmdbuf << ", '" << name << "'";

    if(name.empty()) cmdbuf << " /* nameless */";
    if(pos.empty())  cmdbuf << " /* virtual */";

    cmdbuf << ");\n";
}

static const std::wstring HexNum(const std::wstring& input)
{
    std::wstringstream s;
    unsigned val;

    s.flags(std::ios_base::hex);
    s << input;
    s >> val;

    if(val > 0 && val < 0xFF)
    {
        int diff = val-CurSeqNo;
        if(!diff) return L"a";
        if(diff > -30 && diff < 80)
            return wformat(L"a%+d", diff);
    }
    return wformat(L"0x%X", val);
}

static const std::wstring TranslateCmd(const std::wstring& cmd)
{
    parammap.clear();
    unsigned n=0;
    std::wstring result;
    const wchar_t* fmtptr = cmd.c_str();
    while(*fmtptr)
    {
        if(*fmtptr == L'%')
        {
            std::wstring paramname;
            while(std::isalnum(*++fmtptr)) paramname += *fmtptr;
            std::wstring newname;
            newname += L'a'+(n++);
            parammap[paramname] = newname;
            result += L'%';
            result += newname;
        }
        else
            result += *fmtptr++;
    }
    return result;
}

static void DumpParam(const XMLTreeP treep)
{
    std::wstringstream cmdbuf;

    const XMLTree& tree = *treep;

    const std::wstring pos = tree[L"pos"];
    const std::wstring type = tree[L"type"];
    const std::wstring name = parammap[tree[L"param"]];

    BeginParam(cmdbuf, pos, name);

    if(type == L"Immed")
    {
        std::wstring min = tree[L"min"];
        std::wstring max = tree[L"max"];
        if(min.empty()) min=L"0";
        if(max.empty())
        {
            max = tree[L"size"];
            max = max == L"2" ? L"FFFF" : L"FF";
        }

        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << "," << HexNum(min)
                                  << "," << HexNum(max)
                                  << ",0,0)";
    }
    else if(type == L"ObjectNumber")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ",0x00,0x7E,0,-1)";
    }
    else if(type == L"NibbleHi")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ",0x00,0xFF, 0,0, ElemData::t_nibble_hi)";
    }
    else if(type == L"NibbleLo")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ",0x00,0xFF, 0,0, ElemData::t_nibble_lo)";
    }
    else if(type == L"OrBitNumber")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ",0x00,0x07, 0,0, ElemData::t_orbit)";
    }
    else if(type == L"AndBitNumber")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ",0x00,0x07, 0,0, ElemData::t_andbit)";
    }
    else if(type == L"TableReference")
    {
        std::wstring max = tree[L"size"];
        max = max == L"2" ? L"FFFF" : L"FF";

        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ", 0," << HexNum(max)
                                  << ", " << HexNum(tree[L"address"])
                                  << ",0)";
    }
    else if(type == L"Table2Reference")
    {
        std::wstring max = tree[L"size"];
        max = max == L"2" ? L"FFFF" : L"FF";

        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ", 0," << HexNum(max)
                                  << ", " << HexNum(tree[L"address"])
                                  << ",1)";
    }
    else if(type == L"Prop")
    {
        const std::wstring value = tree[L"value"];
        cmdbuf << "ElemData(" << value.size()/2
                                  << "," << HexNum(value)
                                  << "," << HexNum(value)
                                  << ",0,0)";
    }
    else if(type == L"Const")
    {
        const std::wstring value = tree[L"value"];
        cmdbuf << "ElemData(" << value.size()/2
                                  << "," << HexNum(value)
                                  << "," << HexNum(value)
                                  << ",0,0)";
    }
    else if(type == L"GotoForward")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                   << ",0x00,0xFF,0,0, ElemData::t_else)";
    }
    else if(type == L"GotoBackward")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                   << ",0x00,0xFF,0,0, ElemData::t_loop)";
    }
    else if(type == L"ConditionalGoto")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                   << ",0x00,0xFF,0,0, ElemData::t_if)";
    }
    else if(type == L"Operator")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                   << ",0x00,0x7F,0,0, ElemData::t_operator)";
    }
    else if(type == L"Blob")
    {
        const std::wstring textflag = tree[L"text"];
        if(textflag.empty())
            cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                       << ",0x0000,0xFFFF,0,0, ElemData::t_blob)";
        else
            cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                       << ",0x0000,0xFFFF,0,0, ElemData::t_textblob)";
    }
    else if(type == L"DialogBegin")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ",0x00,0xFFFFFF, 0,0, ElemData::t_dialogbegin)";
    }
    else if(type == L"DialogIndex")
    {
        cmdbuf << "ElemData(" << (std::wstring)tree[L"size"]
                                  << ",0x00,0xFF, 0,0, ElemData::t_dialogaddr)";
    }
    else
    {
        std::wcerr << "Unknown type '" << type << "'\n";
    }

    const std::wstring highbitflag = tree[L"highbit"];
    if(!highbitflag.empty())
    {
        cmdbuf << ".DeclareHighbit()";
    }

    EndParam(cmdbuf, pos, name);

    CurSeq.push_back(cmdbuf.str());
}
static void DumpSequence(const XMLTreeP treep)
{
    const XMLTree& tree = *treep;

    Cmd cmd;
    cmd.cmd  = TranslateCmd(tree[L"cmd"]);
    cmd.size = tree[L"length"];

    CurSeq.clear();
    CurSeqNo = (unsigned) cmds.size();

    tree.ForEach(L"param", DumpParam);

    for(unsigned a=0; a<CurSeq.size(); ++a)
        ops[CurSeq[a]].push_back(CurSeqNo);

    cmds.push_back(cmd);
}

struct bitsetcompare
{
    bool operator() (const std::bitset<MAX_OPS>& a, const std::bitset<MAX_OPS>& b) const
    {
        return a.to_string<char, std::char_traits<char>, std::allocator<char> >()
             < b.to_string<char, std::char_traits<char>, std::allocator<char> >();
    }
};

static void DumpSeqs()
{
    std::wcout << "static const struct { unsigned length; const char *format; } "
                  "ops[" << cmds.size() << "] =\n"
                  "{\n";
    for(unsigned a=0; a<cmds.size(); ++a)
    {
        std::wcout << "\t{ /*" << a << "*/"
                      "  " << cmds[a].size <<
                      ", \"" << cmds[a].cmd << "\""
                      " },\n";
    }
    std::wcout << "};\n";
    std::wcout << "for(unsigned a=0; a<" << cmds.size() << "; ++a) {\n";

    std::wcout << "\tCommand cmd(ops[a].format);\n";
    std::wcout << "\tcmd.SetSize(ops[a].length);\n";

    typedef std::map<std::bitset<MAX_OPS>, std::wstring, bitsetcompare> ops2_t;
    ops2_t ops2;

    for(std::map<std::wstring, std::list<unsigned> >::const_iterator
        i = ops.begin(); i != ops.end(); ++i)
    {
        std::bitset<MAX_OPS> bset;
        for(std::list<unsigned>::const_iterator
            j = i->second.begin();
            j != i->second.end();
            ++j)
        {
            bset.set(*j);
        }
        ops2[bset] += i->first;
    }

    /* Process each set of noncolliding bitsets into switch-cases,
     * until there are none. */
    while(!ops2.empty())
    {
        std::bitset<MAX_OPS> test;

        std::wcout << "\tswitch(a) {\n";
        for(ops2_t::iterator j, i = ops2.begin(); i != ops2.end(); i=j)
        {
            j = i; ++j;

            if((test & i->first).any()) continue;

            for(unsigned a=0; a<cmds.size(); ++a)
                if(i->first.test(a))
                    std::wcout << "\tcase " << a << ":\n";

            std::wcout << i->second;
            std::wcout << "\t\tbreak;\n";

            test |= i->first;

            ops2.erase(i);
        }
        std::wcout << "\t}\n";
    }

    std::wcout << "\tcmd.PutInto(OPTree);\n";
    std::wcout << "\tcmd.PutInto(STRTree);\n";
    std::wcout << "}\n";
}

static void DumpAddress(const XMLTreeP treep)
{
    const XMLTree& tree = *treep;

    std::wstring addr = tree[L"addr"];
    std::wstring name = tree[L"name"];

    if(!addr.empty() && !name.empty())
    {
        std::wcout <<
            "MemoryAddressConstants.Define"
            "(0x" << addr << ", L\"" << name << "\");\n";
    }
}

int main(void)
{
    std::string xml;
    while(!std::feof(stdin))
    {
        char Buf[8192];
        std::size_t n = std::fread(Buf, 1, sizeof Buf, stdin);
        xml += std::string(Buf, n);
    }

    std::wcout << "/* Automatically generated by chronotools/utils/eventsynmake */\n\n";

    XMLTree tree = ParseXML(xml);

    tree[L"location_event_commands"].ForEach(L"sequence", DumpSequence);
    tree[L"location_event_commands"][L"memory_addresses"].ForEach(L"address", DumpAddress);

    DumpSeqs();

    return 0;
}

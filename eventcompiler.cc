#include <vector>
#include <list>
#include <map>

#include "eventcompiler.hh"
#include "eventdata.hh"
#include "base62.hh"
#include "base16.hh"
#include "match.hh"

class LocationEvent
{
public:
    LocationEvent()
    {
    }
    void AddLabel(const std::string& s)
    {
        DefineLabel(s);
    }
    void Parse(const std::wstring& s)
    {
        std::wstring Unknown;
        for(unsigned a=0; a<s.size(); ++a)
        {
            wchar_t wc = s[a];
            char c = WcharToAsc(wc);
            if(std::isspace(c)) continue;
            if(wc != L'[') { Unknown += wc; continue; }
            unsigned begin = a;
            unsigned num_brackets = 0;
            while(a < s.size())
            {
                if(s[a] == L'[') { ++num_brackets; }
                if(s[a] == L']') { --num_brackets; if(!num_brackets) break; }
                ++a;
            }
            if(a >= s.size()) { Unknown += s.substr(begin); break; }

            std::wstring code = s.substr(begin, a+1-begin);
            try
            {
                ParseBlob(code);
            }
            catch(const char* message)
            {
                fprintf(stderr, "ERROR: Can not compile '%s' - '%s'\n",
                    WstrToAsc(code).c_str(), message);
            }
        }
        if(!Unknown.empty())
        {
            fprintf(stderr, "ERROR: Unprocessable event script: '%s'\n",
                WstrToAsc(Unknown).c_str());
        }
    }

    const std::vector<unsigned char> Compile()
    {
        if(!stack.empty())
        {
            fprintf(stderr, "ERROR: Unexpected end of event code\n");
        }
        TraceLabels();
        ConvertToBytes();

        return CreateCode();
    }
private:
    void ParseBlob(const std::wstring& s)
    {
        if(s.empty()) return;
        /*
            * = done
            Metacode to take care of:
             * [OBJECT:<list>]
             * [FUNCTION:<list>]
             * $<label>
             * [Goto:<label>]
             * [Goto:<label> [If<condition>]]        (INVERTED!)
             * [Goto:<label> [Unless<condition>]]
             * [Break]
             * [Break [If<condition>]]               (INVERTED!)
             * [Break [Unless<condition>]]
             * [LoopBegin]
             * [While<condition>]                    (INVERTED!)
             * [Until<condition>]
             * [Loop]
             * [If<condition>]
             * [Unless<condition>]                   (INVERTED!)
             * [ElseIf<condition>]
             * [ElseUnless<condition>]               (INVERTED!)
             * [Else]
             * [EndIf]
         */

        /* Macro for requiring that current context is an 'if' */

        //fprintf(stderr, "Blob '%s'\n", WstrToAsc(s).c_str());

        std::wstring param0, param1;

        if(Match(s, L"[OBJECT:%0]", param0))
        {
            ParseSelectObject(param0);
            return;
        }
        if(Match(s, L"[FUNCTION:%0]", param0))
        {
            ParseSelectPointer(param0);
            return;
        }

        /* Goto anywhere */
        if(Match(s, L"[Goto:%0 [Unless%1]]", param0, param1))
        {
            CreateConditionalGoto(param1, WstrToAsc(param0));
            return;
        }
        if(Match(s, L"[Goto:%0 [If%1]]", param0, param1))
        {
            CreateInverseConditionalGoto(param1, WstrToAsc(param0));
            return;
        }
        if(Match(s, L"[Goto:%0]", param0))
        {
            CreateUnconditionalGoto(WstrToAsc(param0));
            return;
        }

        /* Goto loop end */
        if(Match(s, L"[Break [Unless%0]]", param0))
        {
            const AnalysisState& state = FindLoopState();

            CreateConditionalGoto(param0, state.loopend);
            return;
        }
        if(Match(s, L"[Break [If%0]]", param0))
        {
            const AnalysisState& state = FindLoopState();

            CreateInverseConditionalGoto(param0, state.loopend);
            return;
        }
        if(Match(s, L"[Break]"))
        {
            const AnalysisState& state = FindLoopState();

            CreateUnconditionalGoto(state.loopend);
            return;
        }

        /* Create loop */
        if(Match(s, L"[LoopBegin]"))
        {
            std::string cur_label = CreateLabel();
            std::string end_label = CreateLabel();
            DefineLabel(cur_label);
            EnterLoopState(cur_label, end_label);
            return;
        }
        if(Match(s, L"[Until%0]", param0))
        {
            std::string cur_label = CreateLabel();
            std::string end_label = CreateLabel();
            DefineLabel(cur_label);
            EnterLoopState(cur_label, end_label);
            CreateConditionalGoto(param0, end_label);
            return;
        }
        if(Match(s, L"[While%0]", param0))
        {
            std::string cur_label = CreateLabel();
            std::string end_label = CreateLabel();
            DefineLabel(cur_label);
            EnterLoopState(cur_label, end_label);
            CreateInverseConditionalGoto(param0, end_label);
            return;
        }

        /* End loop */
        if(Match(s, L"[Loop]"))
        {
            const AnalysisState& state = RequireLoopState();

            CreateUnconditionalGoto(state.loopbegin);
            DefineLabel(state.loopend);

            ExitState();
            return;
        }

        /* Create "if" */
        if(Match(s, L"[If%0]", param0))
        {
            std::string elsepos = CreateLabel();
            std::string endif   = CreateLabel();

            EnterIfState(elsepos, endif);
            CreateConditionalGoto(param0, elsepos);
            return;
        }
        if(Match(s, L"[Unless%0]", param0))
        {
            std::string elsepos = CreateLabel();
            std::string endif   = CreateLabel();

            EnterIfState(elsepos, endif);
            CreateInverseConditionalGoto(param0, elsepos);
            return;
        }

        /* Require "if" */
        if(Match(s, L"[Else]"))
        {
            AnalysisState& state = RequireIfState();

            CreateUnconditionalGoto(state.endif);
            DefineLabel(state.elsepos);
            state.has_else = true;
            return;
        }
        if(Match(s, L"[ElseIf%0]", param0))
        {
            AnalysisState& state = RequireIfState();

            // end the current branch
            CreateUnconditionalGoto(state.endif);
            // create the alternate branch
            DefineLabel(state.elsepos);
            // and chain the next label
            state.elsepos  = CreateLabel();
            state.has_else = false;
            CreateConditionalGoto(param0, state.elsepos);
            return;
        }
        if(Match(s, L"[ElseUnless%0]", param0))
        {
            AnalysisState& state = RequireIfState();

            // end the current branch
            CreateUnconditionalGoto(state.endif);
            // create the alternate branch
            DefineLabel(state.elsepos);
            // and chain the next label
            state.elsepos  = CreateLabel();
            state.has_else = false;
            CreateInverseConditionalGoto(param0, state.elsepos);
            return;
        }

        /* End "if" */
        if(Match(s, L"[EndIf]"))
        {
            AnalysisState& state = RequireIfState();

            DefineLabel(state.endif);
            if(!state.has_else) DefineLabel(state.elsepos);

            ExitState();
            return;
        }
        CreateStatement(s);
    }

    void ParseSelectObject(const std::wstring& hexnumlist)
    {
        unsigned num=0;
        ClearObjectSelection();
        for(unsigned a=0; a<=hexnumlist.size(); ++a)
            if(a>=hexnumlist.size() || !CumulateBase16(num, WcharToAsc(hexnumlist[a])))
                { SelectObject(num); num=0; }
    }
    void ParseSelectPointer(const std::wstring& hexnumlist)
    {
        unsigned num=0;
        ClearPointerSelection();
        for(unsigned a=0; a<=hexnumlist.size(); ++a)
            if(a>=hexnumlist.size() || !CumulateBase16(num, WcharToAsc(hexnumlist[a])))
                { SelectPointer(num); num=0; }
    }

    void ClearObjectSelection()
    {
        for(unsigned a=0; a<objects.size(); ++a)
            objects[a].selected = false;
    }
    void SelectObject(unsigned n)
    {
        unsigned top=objects.size();
        if(top <= n) objects.resize(n+1);
        /* Reset new objects */
        ; // nothing
        /* Select the object */
        objects[n].selected = true;
    }
    void ClearPointerSelection()
    {
        for(unsigned a=0; a<objects.size(); ++a)
            for(unsigned b=0; b<16; ++b)
                objects[a].pointers[b].selected=false;
    }
    void SelectPointer(unsigned n)
    {
        if(n >= 16) return;
        for(unsigned a=0; a<objects.size(); ++a)
            if(objects[a].selected)
                objects[a].pointers[n].selected=true;
    }

    struct AnalysisState
    {
        enum { s_loop, s_if } state;
        // loop:
        std::string loopbegin;
        std::string loopend;
        // if:
        std::string elsepos, endif;
        bool has_else;
    };
    void EnterLoopState(const std::string& begin, const std::string& end)
    {
        AnalysisState newstate;
        newstate.state = AnalysisState::s_loop;
        newstate.loopbegin = begin;
        newstate.loopend   = end;
        stack.push_front(newstate);
    }
    void EnterIfState(const std::string& elsepos, const std::string& endif)
    {
        AnalysisState newstate;
        newstate.state    = AnalysisState::s_if;
        newstate.elsepos  = elsepos;
        newstate.endif    = endif;
        newstate.has_else = false;
        stack.push_front(newstate);
    }
    void ExitState()
    {
        stack.erase(stack.begin());
    }
    AnalysisState& RequireIfState()
    {
        if(stack.empty() || stack.begin()->state != AnalysisState::s_if)
        {
            throw "No conditional context";
        }
        return *stack.begin();
    }
    const AnalysisState& RequireLoopState() const
    {
        if(stack.empty() || stack.begin()->state != AnalysisState::s_loop)
        {
            throw "No loop context";
        }
        return *stack.begin();
    }
    const AnalysisState& FindLoopState() const
    {
        for(std::list<AnalysisState>::const_iterator
            i=stack.begin(); i!=stack.end(); ++i)
        {
            if(i->state == AnalysisState::s_loop)
                return *i;
        }
        throw "No loop context";
    }

    const std::string CreateLabel() const
    {
        static unsigned LabelCounter = 0;
        return "!"+EncodeBase62(LabelCounter++, 0);
    }

    void CreateUnconditionalGoto(const std::string& label)
    {
        CreateStatement(L"", label);
    }
    void CreateConditionalGoto(const std::wstring& condition, const std::string& label)
    {
        CreateStatement(condition, label);
    }
    void CreateInverseConditionalGoto(const std::wstring& condition, const std::string& label)
    {
        std::string tmplabel = CreateLabel();
        CreateConditionalGoto(condition, tmplabel);
        CreateUnconditionalGoto(label);
        DefineLabel(tmplabel);
    }
    void DefineLabel(const std::string& label)
    {
        // Mark the label pointing to the next line
        if(labels.find(label) != labels.end())
        {
            fprintf(stderr, "Error: Duplicate definition of label '%s'\n", label.c_str());
        }
        labels[label] = lines.size();
    }
    void CreateStatement(const std::wstring& text, const std::string& goto_label="")
    {
        Line line;
        line.text       = text;
        line.goto_label = goto_label;

        unsigned linenumber = lines.size();

        for(unsigned a=0; a<objects.size(); ++a)
            if(objects[a].selected)
                for(unsigned b=0; b<16; ++b)
                    if(objects[a].pointers[b].selected)
                    {
                        //fprintf(stderr, "Object %u pointer %u line=%u\n", a, b, linenumber);
                        objects[a].pointers[b].linenumber = linenumber;
                    }

        /*
        std::wstring msg = L"Created";
        for(std::set<std::string>::const_iterator
            i = line.names.begin(); i != line.names.end(); ++i)
        {
            msg += L" <" + AscToWstr(*i) + L"> ";
        }
        msg += L"(" + text + L")";
        if(!goto_label.empty())
            msg += L" -> " + AscToWstr(goto_label);

        fprintf(stderr, "%s\n", WstrToAsc(msg).c_str());
        */
        lines.push_back(line);

        ClearPointerSelection();
    }

    unsigned FindLabel(const std::string& s) const
    {
        std::map<std::string, unsigned>::const_iterator i = labels.find(s);
        if(i == labels.end())
        {
            fprintf(stderr, "ERROR: Undefined label '%s'\n", s.c_str());
            return 0;
        }
        return i->second;
    }

    void TraceLabels()
    {
        for(unsigned a=0; a<lines.size(); ++a)
        {
            const std::string& l = lines[a].goto_label;
            if(l.empty()) continue;

            unsigned n = FindLabel(l);
            lines[a].goto_target   = n;
            lines[a].goto_backward = n <= a;
            /* Don't clear the goto label.
             * It is used for patching gotos.
             */
        }
        labels.clear(); // Free unneeded data
    }

    void ConvertToBytes()
    {
        EventCode com;

        const unsigned code_start = 1 + objects.size() * 16 * 2;
        unsigned addr = code_start;

        for(unsigned a=0; a<lines.size(); ++a)
        {
            Line& l = lines[a];

            l.begin_address = addr;

            try
            {
                EventCode::EncodeResult tmp = com.EncodeCommand(l.text, l.goto_backward);
                l.bytes       = tmp.result;
                l.goto_offset = tmp.goto_position;
                addr += l.bytes.size();
            }
            catch(bool)
            {
                fprintf(stderr, "ERROR: '%s' has no bytecode representation\n",
                    WstrToAsc(l.text).c_str());
            }
        }
    }

    const std::vector<unsigned char> CreateCode() const
    {
        std::vector<unsigned char> result;

        result.reserve(0x1700); // maximum size of the event

        result.push_back(objects.size());
        for(unsigned a=0; a<objects.size(); ++a)
            for(unsigned b=0; b<16; ++b)
            {
                unsigned nr = objects[a].pointers[b].linenumber;
                unsigned offs = lines[nr].begin_address - 1;
                result.push_back(offs & 0xFF);
                result.push_back(offs >> 8);
            }

        for(unsigned a=0; a<lines.size(); ++a)
        {
            const Line& l = lines[a];

            unsigned resultpos = result.size();
            result.insert(result.end(), l.bytes.begin(), l.bytes.end());

            if(!l.goto_label.empty())
            {
                unsigned offset = l.begin_address + l.goto_offset;

                if(l.goto_backward)
                    offset = offset - lines[l.goto_target].begin_address;
                else
                    offset = lines[l.goto_target].begin_address - offset;

                if(offset > 255)
                {
                    fprintf(stderr, "ERROR: Goto is too far\n");
                }
                result[resultpos + l.goto_offset] = offset;
            }
        }
        return result;
    }

    struct Line
    {
        /* the encoded data */
        std::vector<unsigned char> bytes;
        unsigned     goto_offset;   //where goto is in
        unsigned     begin_address; //the address of this code

        /* the code, as in source or as generated (text form) */
        std::wstring text;
        std::string  goto_label;
        unsigned     goto_target; //line number
        bool         goto_backward;
    };

    struct Pointer
    {
        unsigned linenumber;
        bool selected;
    public:
        Pointer(): linenumber(0),selected(false) { }
    };
    struct Object
    {
        /* Pointers pointing to 16 locations
         * in the lines[] array.
         */
        Pointer pointers[16];
        bool selected;
    public:
        Object(): selected(false) { }
    };


    /* Compiled data */
    std::vector<Line> lines;
    std::vector<Object> objects;

    /* Parsed data */
    std::map<std::string, unsigned> labels;
    std::list<AnalysisState> stack;
};


EventCompiler::EventCompiler(): last_addr(0), cur_ev(NULL)
{
}

EventCompiler::~EventCompiler()
{
    delete cur_ev;
}

void EventCompiler::AddData
    (unsigned ptr_addr,
     const std::string& label,
     const std::wstring& data)
{
    if(last_addr != ptr_addr)
    {
        FinishCurrent();
        cur_ev = new LocationEvent;
    }
    last_addr = ptr_addr;
    cur_ev->AddLabel(label);
    cur_ev->Parse(data);
}

void EventCompiler::FinishCurrent()
{
    if(!cur_ev) return;

    /*
        //test
        EventCode tmp2; tmp2.EncodeCommand(L"[ObjectLetB:{Result}:7F0242]", false);
    */

    Event tmp;
    tmp.ptr_addr = last_addr;
    tmp.data     = cur_ev->Compile();
    events.push_back(tmp);

    delete cur_ev; cur_ev=NULL;
}

void EventCompiler::Close()
{
    FinishCurrent();
}

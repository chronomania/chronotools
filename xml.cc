#include <algorithm>
#include <iostream>
#include <cctype>
#include "xml.hh"

namespace
{
    typedef const struct { } xml_error;   // for throwing
    typedef const struct { } xml_end_tag; // for throwing
    
    bool is_tag_char(char c)
    {
        return std::isalnum(c)
            || c == '_' || c == ':'
            || c == '-' || c == '.';
    }
    
    wchar_t AscToWchar(char c)
    {
        return c;
    }

    std::wstring ParseEntity(const std::string& s, unsigned& pos)
    {
        if(s.substr(pos, 5) == "&amp;"    ) { pos += 5; return L"&"; }
        if(s.substr(pos, 6) == "&quot;"   ) { pos += 6; return L"\""; }
        if(s.substr(pos, 4) == "&lt;"     ) { pos += 4; return L"<"; }
        if(s.substr(pos, 4) == "&gt;"     ) { pos += 4; return L">"; }
        if(s.substr(pos, 2) == "&#")
        {
            const char* begin = s.c_str()+pos;
            const char* end;
            wchar_t code = std::strtol(begin+2, (char**)&end, 10);
            std::wstring result; result += code;
            pos += (end-begin) + 2;
            if(s[pos] == ';') ++pos;
            return result;
        }
        std::wstring result; result += AscToWchar(s[pos++]);
        return result;
    }
    
    void ParseSubTree(XMLTree& result, const std::string& s, unsigned& pos);
    
    void ParseTag(XMLTree& result, const std::string& s, unsigned& pos)
    {
        std::wstring tagname;
        bool begin_slash = false;
        bool meta_tag = false;

        //std::cerr << "input (" << s.substr(pos, 100) << ")\n";
        
        if(pos < s.size() && s[pos] == '/') { begin_slash=true; ++pos; }
        if(pos < s.size() && s[pos] == '?') { meta_tag=true; ++pos; }
        if(pos < s.size() && s[pos] == '!')
        {
            ++pos;
            bool is_comment = false;
            for(;;)
            {
                while(pos < s.size() && std::isspace(s[pos])) ++pos;
                if(pos >= s.size()) throw xml_error();
                if(pos+2 <= s.size() && s[pos]=='-' && s[pos+1]=='-')
                {
                    pos += 2;
                    is_comment = !is_comment;
                    continue;
                }
                if(s[pos] == '>' && !is_comment) { ++pos; break; }
                ++pos;
            }
            return;
        }
        
        XMLTreeP subtree = new XMLTree;
        
        while(pos < s.size() && is_tag_char(s[pos]))
            tagname += AscToWchar(s[pos++]);

        //std::wcerr << L"tag: " << tagname << L"\n";
    
        // Loop until the end of the tag has been reached
        bool found_trailing_slash = begin_slash;
        for(;;)
        {
            while(pos < s.size() && std::isspace(s[pos])) ++pos;
            if(pos >= s.size()) throw xml_error();
            if(meta_tag && s[pos] == '?') { ++pos; continue; }
            if(s[pos] == '>') { ++pos; break; }
            
            if(s[pos] == '/' && !found_trailing_slash)
                { found_trailing_slash=true; ++pos; continue; }
            if(!is_tag_char(s[pos])) throw xml_error();
            
            std::wstring tagparamname;
            while(pos < s.size() && is_tag_char(s[pos]))
                tagparamname += s[pos++];
            
            //std::wcerr << L"param: " << tagparamname << L"\n";
            
            while(pos < s.size() && std::isspace(s[pos])) ++pos;
            if(pos >= s.size()) throw xml_error();
            if(s[pos] != '=') throw xml_error();  ++pos;
            
            while(pos < s.size() && std::isspace(s[pos])) ++pos;
            if(pos >= s.size()) throw xml_error();
            
            if(s[pos] != '"' && s[pos] != '\'') throw xml_error();
            char terminator = s[pos++];
            
            std::wstring tagparamvalue;
            for(;;)
            {
                if(pos >= s.size()) throw xml_error();
                if(s[pos] == terminator) { ++pos; break; }
                tagparamvalue += ParseEntity(s, pos);
            }
            
            subtree->params.insert(std::make_pair(tagparamname, tagparamvalue));
            
            //std::wcerr << "Got param (" << tagparamname << ")(" << tagparamvalue << ")\n";
        }
        
        if(begin_slash)
        {
            throw xml_end_tag();
        }
        
        if(!found_trailing_slash && !meta_tag)
            ParseSubTree(*subtree, s, pos);
        
        if(!meta_tag)
            result.tags.insert(std::make_pair(tagname, subtree));
    }
    
    void ParseSubTree(XMLTree& result, const std::string& s, unsigned& pos)
    {
        std::wstring spaces;
        while(pos < s.size())
        {
            if(std::isspace(s[pos]))
            {
                if(!result.value.empty()) spaces += AscToWchar(s[pos]);
                ++pos;
                continue;
            }
            if(s[pos] == '<')
            {
                ++pos;
                try
                {
                    ParseTag(result, s, pos);
                }
                catch(xml_end_tag)
                {
                    return;
                }
                continue;
            }
            result.value += spaces;
            spaces = L"";
            result.value += ParseEntity(s, pos);
        }
    }
}

const XMLTree ParseXML(const std::string& s)
{
    XMLTree result;
    try
    {
        unsigned pos=0;
        ParseSubTree(result, s, pos);
    }
    catch(xml_error err)
    {
        //FIXME: what now?
    }
    return result;
}

XMLTreeSet XMLTree::operator[] (const std::wstring& key) const
{
    XMLTreeSet result;
    
    typedef tagmap_t::const_iterator tagcit;
    std::pair<tagcit, tagcit> tmp = tags.equal_range(key);
    
    for(tagcit i=tmp.first; i!=tmp.second; ++i)
        result.AddTree(i->second);
    
    typedef parammap_t::const_iterator paramcit;
    std::pair<paramcit, paramcit> tmp2 = params.equal_range(key);
    
    for(paramcit i=tmp2.first; i!=tmp2.second; ++i)
        result.SetValue(i->second);

    return result;
}

const XMLTreeSet XMLTreeSet::operator[] (const std::wstring& key) const
{
    XMLTreeSet result;
    
    for(std::list<XMLTreeP>::const_iterator
        i = matching_trees.begin();
        i != matching_trees.end();
        ++i)
    {
        const XMLTree& p = *(*i);
        result.Combine( p[key] );
    }
    return result;    
}

void XMLTreeSet::AddTree(XMLTreeP p)
{
    matching_trees.push_back(p);
    matching_strings.push_back(p->value);
}

void XMLTreeSet::SetValue(const std::wstring& s)
{
    matching_strings.push_back(s);
}

void XMLTreeSet::Combine(const XMLTreeSet& b)
{
    matching_trees.insert(matching_trees.end(),     b.matching_trees.begin(),   b.matching_trees.end());
    matching_strings.insert(matching_strings.end(), b.matching_strings.begin(), b.matching_strings.end());
}

XMLTreeSet::operator const std::wstring () const
{
    if(matching_strings.empty()) return L"";
    return *matching_strings.begin();
}

/*
XMLTreeSet::operator const XMLTreeP () const
{
    if(matching_trees.empty()) return new XMLTree;
    return *matching_trees.begin();
}
*/

void XMLTreeSet::Iterate(void (*func)(XMLTreeP)) const
{
    std::for_each(matching_trees.begin(), matching_trees.end(), func);
}

void XMLTreeSet::Iterate(void (*func)(const std::wstring&)) const
{
    std::for_each(matching_strings.begin(), matching_strings.end(), func);
}

#include <cstdio>
#include <string>
#include <list>
#include <map>

using namespace std;

struct item
{
    string text;
    string section;
};

class File
{
    map<string, item> keys;
    list<string> script;
    
    void RemoveLastLF(string& content)
    {
        unsigned endpos = content.size();
        while(endpos > 0 && content[endpos-1] == '\n') --endpos;
        content.erase(endpos, content.size());
    }
    
    void AddEntry(const string& sect, string& label, string& content)
    {
        RemoveLastLF(content);
        
        item entry;
        entry.text    = content;
        entry.section = sect;
        keys[label] = entry;
        
        script.push_back(label);
        
        //fprintf(stderr, "registered %s in %s\n", label.c_str(), sect.c_str());
        
        label   = "";
        content = "";
    }
    void AddComment(string& content)
    {
        script.push_back(content);
        
        //fprintf(stderr, "added comment: %s\n", s);
        
        content = "";
    }
    
    void ShowComment(const string& s)
    {
        printf("%s", s.c_str());
    }
    void ShowEntry(const string& sect, const string& label, const string& content)
    {
        static string LastSect;
        if(sect != LastSect)
        {
            printf("\n\n*%s\n", sect.c_str());
            LastSect = sect;
        }
        printf("$%s:%s", label.c_str(), content.c_str());
        printf("\n");
    }
    
    void Load(const string& filename)
    {
        FILE *fp = fopen(filename.c_str(), "rt");
        
        enum
        {
            st_0,
            st_section,
            st_label,
            st_data
        } state = st_0;
        
        string secthdr;
        string label;
        string content;
        
        string comment_pending;
        
        for(;;)
        {
            char Buf[512];
            if(!fgets(Buf, sizeof Buf, fp)) break;
            
            char *s = Buf;
            while(*s)
            {
                if(*s == '\r') { ++s; continue; }
                
                switch(state)
                {
                    case st_0:       // before first *
                    {
                        if(*s == ';')
                        {
                    Is_Comment:
                            comment_pending += s;
                            s = ""; break;
                        }
                        if(*s == '$')
                        {
                            fprintf(stderr, "Syntax error: %c unexpected\n", *s);
                            return;
                        }
                        if(*s == '*')
                        {
                    Is_Section:
                            if(!comment_pending.empty()) AddComment(comment_pending);
                            strtok(s, ";\r\n");
                            secthdr = s+1;
                            state = st_section;
                            s = "";
                            break;
                        }
                        fprintf(stderr, "%c", *s++);
                        break;
                    }
                    case st_section: // after *xx but before $
                    {
                        if(*s == ';')
                        {
                            goto Is_Comment;
                        }
                        if(*s == '*')
                        {
                            goto Is_Section;
                        }
                        if(*s == '$')
                        {
                    Is_Label:
                            if(!comment_pending.empty())
                            {
                                AddComment(comment_pending);
                            }
                            state = st_label;
                            label = "";
                            ++s;
                            break;
                        }
                        fprintf(stderr, "%c", *s++);
                        break;
                    }
                    case st_label:  // after $
                    {
                        if(*s == ':')
                        {
                            if(!comment_pending.empty()) AddComment(comment_pending);
                            state   = st_data;
                            content = "";
                            ++s;
                            break;
                        }
                        if(*s == ';' || *s == '*')
                        {    
                            fprintf(stderr, "Syntax error: %c unexpected\n", *s);
                            return;
                        }
                        label += *s++;
                        break;
                    }
                    case st_data:
                    {
                        if(*s == ';')
                        {
                            goto Is_Comment;
                        }
                        if(*s == '*')
                        {
                            if(!label.empty()) AddEntry(secthdr, label, content); // flush
                            if(!comment_pending.empty()) AddComment(comment_pending);
                            
                            goto Is_Section;
                        }
                        if(!comment_pending.empty())
                        {
                            //RemoveLastLF(content);
                            content += comment_pending;
                            comment_pending = "";
                        }
                        if(*s == '$')
                        {
                            if(!label.empty()) AddEntry(secthdr, label, content); // flush
                            if(!comment_pending.empty()) AddComment(comment_pending);
                            goto Is_Label;
                        }
                        content += *s++;
                        break;
                    }
                }
            }
        }
        
        if(!label.empty()) AddEntry(secthdr, label, content); // flush
        if(!comment_pending.empty()) AddComment(comment_pending);
        
        fclose(fp);
    }
public:
    File(const string& fn)
    {
        Load(fn);
    }
    
    void Format(File& b)
    {
        if(script.empty()) return;
        
        list<string>::const_iterator i;
        map<string, item>::iterator k;
        
        for(i = script.begin(); i != script.end(); ++i)
        {
            const string& s = *i;
            if(s[0] == ';')
            {
                ShowComment(s);
                continue;
            }
            k = b.keys.find(s);
            if(k == b.keys.end())
            {
                k = keys.find(s);
                if(k == keys.end()) continue;
            }
            ShowEntry(k->second.section,
                      k->first,
                      k->second.text);
            
            b.keys.erase(s);
            keys.erase(s);
        }
        script.clear();
        
        b.Format(*this);
    }
};

int main(void)
{
    File source("FIN/ct.txt");
    File sample("tmp/ctdump.out");
    
    sample.Format(source);
}

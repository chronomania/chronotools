#include <map>
#include <list>

#include "wstring.hh"
#include "autoptr"

typedef autoptr<class XMLTree> XMLTreeP;

class XMLTree: public ptrable
{
public:
    // tags
    typedef std::multimap<std::wstring, XMLTreeP> tagmap_t;
    tagmap_t tags;
    // current tag params
    typedef std::map<std::wstring, std::wstring> parammap_t;
    parammap_t params;
    // tag value
    std::wstring value;

    class XMLTreeSet operator[] (const std::wstring&) const;

    template<typename Func>
    void ForEach(const std::wstring& key, Func f) const;
};

const XMLTree ParseXML(const std::string& s);

class XMLTreeSet
{
public:
    const XMLTreeSet operator[] (const std::wstring&) const;

    operator const std::wstring () const;
    //operator const XMLTreeP () const;

    template<typename Func>
    void ForEach(const std::wstring& key, Func f) const
    {
        for(std::list<XMLTreeP>::const_iterator
            i = matching_trees.begin();
            i != matching_trees.end();
            ++i)
        {
            const XMLTree& p = *(*i);
            p[key].Iterate(f);
        }
    }

    void Iterate(void (*func)(XMLTreeP)) const;
    void Iterate(void (*func)(const std::wstring& )) const;

protected:
    friend class XMLTree;
    void AddTree(XMLTreeP p);
    void SetValue(const std::wstring& s);
    void Combine(const XMLTreeSet& b);

private:
    std::list<XMLTreeP> matching_trees;
    std::list<std::wstring> matching_strings;
};

template<typename Func>
void XMLTree::ForEach(const std::wstring& key, Func f) const
{
    (*this)[key].Iterate(f);
}

#include <vector>
#include <string>
#include <list>

class PagePtrList
{
public:
    void AddItem(const std::vector<unsigned char>&, unsigned short ptraddr);
    
    void Create(class insertor& ins,
                unsigned char page,
                const std::string& what,
                const std::string& tablename = "");

    void Create(class insertor& ins,
                const std::string& what,
                unsigned original_address,
                const std::string& tablename = "");

    void Create(class insertor& ins,
                const std::string& what,
                const std::string& tablename = "");

    void Combine();
    
    unsigned Size() const;
    const std::vector<unsigned char> GetS() const;

public:
    PagePtrList(): items() { }
    
private:
    struct PagePtrInfo;
    void Create(class insertor& ins,
                const std::string& what,
                const std::string& tablename,
                const struct PagePtrInfo& info);

    friend class PagePtrListFriend;
    
    struct Reference
    {
        unsigned short ptraddr; /* Where the pointer is */
        unsigned offset;        /* Which position in the string does it refer to */
        
        Reference(unsigned short ptr): ptraddr(ptr), offset(0) {}
    };
    struct Data
    {
        std::vector<unsigned char> data;
        std::list<Reference> refs;
        
        Data(const std::vector<unsigned char>& d,
             unsigned short r): data(d), refs(1, r) {}
    };
    std::list<Data> items;
private:
    void Combine(Data& a, const Data& b, unsigned offset) const;
};

#include <vector>
#include <string>
#include <list>

class PagePtrList
{
public:
    void AddItem(const std::vector<unsigned char>&, unsigned short ptraddr);
#if 0
    void AddItem(const std::string&, unsigned short ptraddr);
#endif
    void Create(class insertor& ins,
                int page,
                const std::string& what,
                const std::string& tablename = "");

    void Create(class insertor& ins,
                const std::string& what,
                const std::string& tablename = "");

    void Combine();
    
    unsigned Size() const;
    const std::vector<unsigned char> GetS() const;

private:
    struct Reference
    {
        unsigned short ptraddr;
        unsigned offset;
        
        Reference(unsigned short ptr): ptraddr(ptr), offset(0) {}
    };
    struct Data
    {
        std::vector<unsigned char> data;
        std::list<Reference> refs;
        
        Data(const std::vector<unsigned char>& d,
             unsigned short r): data(d), refs(1, r) {}
        
        bool operator< (const Data& ) const;
    };
    std::list<Data> items;
private:
    void Combine(Data& a, const Data& b, unsigned offset) const;
};

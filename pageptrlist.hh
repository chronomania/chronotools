#include <vector>
#include <string>

using std::vector;

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

private:
    void Combine();

    struct Reference
    {
        unsigned short ptraddr;
        unsigned offset;
        
        Reference(unsigned short ptr): ptraddr(ptr), offset(0) {}
    };
    struct Data
    {
        vector<unsigned char> data;
        vector<Reference> refs;
        
        Data(const std::vector<unsigned char>& d,
             unsigned short r): data(d), refs(1, r) {}
        
        void Combine(Data& b, unsigned offset);

        bool operator< (const Data& ) const;
    };
    std::vector<Data> items;
};

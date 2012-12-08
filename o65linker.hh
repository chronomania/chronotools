#ifndef bqtO65linkerHH
#define bqtO65linkerHH

#include <vector>
#include <utility>
#include <cstdio>
#include <string>

#include "o65.hh"
#include "refer.hh"

#define IPS_ADDRESS_EXTERN 0x01
#define IPS_ADDRESS_GLOBAL 0x02
#define IPS_EOF_MARKER     0x454F46

/* All addresses used by the linker are SNES-based, not ROM-based */

struct LinkageWish
{
    enum type
    {
        LinkAnywhere,
        LinkHere,
        LinkInGroup,
        LinkThisPage
    } type;
    unsigned param;
public:
    LinkageWish(): type(LinkAnywhere), param(0) {}

    unsigned GetAddress() const
    {
        if(type != LinkHere) return 0;
        return param;
    }
    unsigned GetPage() const { return param; }
    unsigned GetGroup() const { return param; }

    void SetAddress(unsigned addr) { param=addr; type=LinkHere; }
    void SetLinkageGroup(unsigned num) { param=num; type=LinkInGroup; }
    void SetLinkagePage(unsigned page) { param=page; type=LinkThisPage; }
};

/**
 * Linker of O65 objects (and other blobs) in SNES address space.
 */
class O65linker
{
public:
    O65linker();
    ~O65linker();

    void LoadIPSfile(std::FILE* fp, const std::string& what);

    /*! Adds an O65 object file with given linkage type */
    void AddObject(const O65& object, const std::string& what, LinkageWish linkage = LinkageWish());

    /*! Adds an O65 object file that will be statically stored to the given address */
    void AddObject(const O65& object, const std::string& what, unsigned address);

    /*! Adds a lump of data that will be stored to the given address */
    /*! If a public symbol is given, it points to the beginning of the lump */
    /*! "what" describes the object, but is not used in linking. */
    void AddLump(const std::vector<unsigned char>&,
                 unsigned address,
                 const std::string& what,
                 const std::string& name="");

    /*! Adds a lump of data that will be stored anywhere it fits */
    /*! "what" describes the object, but is not used in linking. */
    /*! A name must be given. */
    void AddLump(const std::vector<unsigned char>&,
                 const std::string& what,
                 const std::string& name);

    /*! Creates a lump that referers to the given symbol at given address. */
    /*! The address is defined in the reference object. */
    void AddReference(const std::string& name, const ReferMethod& reference);

    /*! Creates a new linkage group. */
    unsigned CreateLinkageGroup() { return ++num_groups_used; }

    /*! Gets the list of sizes for all lumps/objects */
    const std::vector<unsigned> GetSizeList(SegmentSelection seg = CODE) const;

    /*! Gets the list of addresses for all lumps/objects */
    /*! If the object is not yet linked, 0 is shown */
    const std::vector<unsigned> GetAddrList(SegmentSelection seg = CODE) const;

    /*! Gets the linkage type for all lumps/objects */
    const std::vector<LinkageWish> GetLinkageList(SegmentSelection seg = CODE) const;

    /*! Defines the addresses for all lumps/objects */
    void PutAddrList(const std::vector<unsigned>& addrs, SegmentSelection seg = CODE);

    /*! Gets the contents of the given lump/object */
    const std::vector<unsigned char>& GetCode(unsigned objno) const;

    /*! Gets the name ("what") of the given lump/object */
    const std::string& GetName(unsigned objno) const;

    /*! Defines a symbol */
    void DefineSymbol(const std::string& name, unsigned value);

    /*! Links all modules. If there exists a lump/object with
     * no known address, the behavior is unknown.
     */
    void Link();

    /*! Sorts the objects by address */
    void SortByAddress();

    /*! Releases the memory allocated by given object. */
    void Release(unsigned objno); // no range checks

private:
    // Copying prohibited
    O65linker(const O65linker& );
    const O65linker& operator= (const O65linker& );

private:
    void FinishAndDeleteReference(unsigned refnum, unsigned value);

    void FinishReference(const ReferMethod& reference,
                         unsigned target,
                         const std::string& what);

    class SymCache;
    class Object;
    class definedata;
    class referdata;

    SymCache *symcache;

    std::vector<Object* > objects;
    std::vector<definedata *> defines;
    std::vector<referdata *> referers;
    unsigned num_groups_used;
    bool linked;
};

#endif

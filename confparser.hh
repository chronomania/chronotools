#ifndef CONFPARSER_HH_
#define CONFPARSER_HH_

#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

using std::FILE;
using std::map;

#include "wstring.hh"

class ConfParser
{
 public:
    class invalid_section //: public std::runtime_error
    {
       std::string sect;
    public:
       explicit invalid_section(const std::string& arg)
         : /*runtime_error(arg), */sect(arg) {}
       const std::string& GetSection() const { return sect; }
    };
    class invalid_field //: public std::runtime_error
    {
       std::string sect, field;
    public:
       explicit invalid_field(const std::string& sectname,
                              const std::string& fieldname)
        : /*runtime_error(sectname + ":" + fieldname),
          */sect(sectname), field(fieldname) {}
       const std::string& GetSection() const { return sect; }
       const std::string& GetField() const { return field; }
    };

    void Parse(FILE *fp);
    void Clear();

    class Section;
    const Section& operator[](const std::string& sectionName) const;


    class Field
    {
     public:
        struct Element
        {
            unsigned IField;
            double DField;
            ucs4string SField;

            operator bool() const { return IField; }
            operator unsigned() const { return IField; }
            operator double() const { return DField; }
            operator const ucs4string& () const { return SField; }
         public:
            Element(): IField(0), DField(0), SField() { }
        };

        unsigned IField() const;
        double DField() const;
        const ucs4string& SField() const;

        const std::vector<Element>& Fields() const { return elements; }
        
        operator bool() const { return IField(); }
        operator double() const { return DField(); }
        operator unsigned() const { return IField(); }
        operator const ucs4string& () const { return SField(); }

     private:
        friend class ConfParser;
        std::vector<Element> elements;

        void append(const Field& f);
     public:
        Field(): elements() { }
    };

    typedef std::vector<Field::Element> ElemVec;

    class Section
    {
     public:
        const Field& operator[](const std::string& fieldName) const;

     private:
        friend class ConfParser;
        std::string SectName;
        typedef map<std::string, Field> FieldMap;
        FieldMap fields;
     public:
        Section(): SectName(), fields() { }
    };



//-----------------------------------------------------------------------
 private:
    class CharIStream;
    
    void ParseSection(class CharIStream& is, const std::string& secName);
    bool ParseField(class CharIStream& is, Field& field, bool);

    typedef map<std::string, Section> SecMap;
    SecMap sections;
 public:
    ConfParser(): sections() { }
};

#endif

#ifndef CONFPARSER_HH_
#define CONFPARSER_HH_

#include <cstdio>
#include <string>
#include <map>
#include <vector>

using std::FILE;

#include "wstring.hh"

class ConfParser
{
 public:
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
            wstring SField;

            operator bool() const { return IField; }
            operator unsigned() const { return IField; }
            operator const wstring& () const { return SField; }
        };

        unsigned IField() const;
        const wstring& SField() const;

        const std::vector<Element>& Fields() const { return elements; }
        
        operator bool() const { return IField(); }
        operator unsigned() const { return IField(); }
        operator const wstring& () const { return SField(); }

     private:
        friend class ConfParser;
        std::vector<Element> elements;

        void append(const Field& f);
    };

    typedef std::vector<Field::Element> ElemVec;

    class Section
    {
     public:
        const Field& operator[](const std::string& fieldName) const;

     private:
        friend class ConfParser;
        std::string SectName;
        typedef std::map<std::string, Field> FieldMap;
        FieldMap fields;
    };



//-----------------------------------------------------------------------
 private:
    class CharIStream;
    
    void ParseSection(class CharIStream& is, const std::string& secName);
    bool ParseField(class CharIStream& is, Field& field, bool);

    typedef std::map<std::string, Section> SecMap;
    SecMap sections;
};

#endif
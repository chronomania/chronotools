#include <string>

#include "wstring.hh"
#include "ctcset.hh"

void MessageLoadingDialog();
void MessageLoadingImages();
void MessageApplyingDictionary();
void MessageMeasuringScript();
void MessageReorganizingFonts();
void MessageLinkingModules(unsigned count);
void MessageWritingStrings();
void MessageWritingDict();

void MessageLoadingItem(const std::string& header);
void MessageZSection(const std::string& header);
void MessageRSection(const std::string& header);
void MessageLSection(const std::string& header);
void MessageDSection(const std::string& header);
void MessageSSection(const std::string& header);
void MessageESection(const std::string& header);
void MessageC8Section(const std::string& header);
void MessageUnknownHeader(const std::string& header);
void MessageInvalidLabelChar(wchar_t c, unsigned label, const std::string& header);
void MessageInvalidLabelChar(wchar_t c, const std::string& label, const std::string& header);
void MessageWorking();
void MessageUnexpected(const std::wstring& unexpected);
void MessageDone();
void MessageSymbolIgnored(const std::wstring& symbol);
void MessageTFStartError(const std::wstring& tf);
void MessageTFEndError(const std::wstring& tf);
void MessageTooLongText(const ctstring& input, const ctstring& output);
void MessageModuleWithoutAddress(const std::string& name);
void MessageUndefinedSymbol(const std::string& name, const std::string& modname);
void MessageDuplicateDefinition(const std::string& name, unsigned nmods, unsigned ndefs);
void MessageUndefinedSymbols(const std::string& modname, unsigned n);

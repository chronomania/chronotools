#include <string>

#include "wstring.hh"
#include "ctcset.hh"

void MessageLoadingDialog();
void MessageLoadingImages();
void MessageApplyingDictionary();
void MessageMeasuringScript();
void MessageReorganizingFonts();
void MessageLinkingModules(unsigned count);

void MessageLoadingItem(const string& header);
void MessageZSection(const string& header);
void MessageRSection(const string& header);
void MessageLSection(const string& header);
void MessageDSection(const string& header);
void MessageSSection(const string& header);
void MessageUnknownHeader(const string& header);
void MessageInvalidLabelChar(ucs4 c, unsigned label, const string& header);
void MessageInvalidLabelChar(ucs4 c, const string& label, const string& header);
void MessageWorking();
void MessageUnexpected(const ucs4string& unexpected);
void MessageDone();
void MessageSymbolIgnored(const ucs4string& symbol);
void MessageTFStartError(const ucs4string& tf);
void MessageTFEndError(const ucs4string& tf);
void MessageTooLongText(const ctstring& input, const ctstring& output);

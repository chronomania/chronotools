#include <string>

const std::string EncodeBase16(unsigned n, unsigned min_digits=4);

const bool CumulateBase16(unsigned& value, char c);

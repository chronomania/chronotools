#include <string>

const std::string EncodeBase62(unsigned n, unsigned min_digits=4);

const bool CumulateBase62(unsigned& value, char c);

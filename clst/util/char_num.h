#ifndef CLST_UTIL_CHAR_NUM_H
#define CLST_UTIL_CHAR_NUM_H

namespace clst::util
{

const char* char_num(const char* str, char& out, int base=0) noexcept;

const char* char_num(const char* str, signed char& out, int base=0) noexcept;
const char* char_num(const char* str, short& out, int base=0) noexcept;
const char* char_num(const char* str, int& out, int base=0) noexcept;
const char* char_num(const char* str, long& out, int base=0) noexcept;
const char* char_num(const char* str, long long& out, int base=0) noexcept;

const char* char_num(const char* str, unsigned char& out, int base=0) noexcept;
const char* char_num(const char* str, unsigned short& out, int base=0) noexcept;
const char* char_num(const char* str, unsigned int& out, int base=0) noexcept;
const char* char_num(const char* str, unsigned long& out, int base=0) noexcept;
const char* char_num(const char* str, unsigned long long& out, int base=0) noexcept;

const char* char_num(const char* str, float& out) noexcept;
const char* char_num(const char* str, double& out) noexcept;
const char* char_num(const char* str, long double& out) noexcept;

} // namespace clst::util

#endif /* CLST_UTIL_CHAR_NUM_H */

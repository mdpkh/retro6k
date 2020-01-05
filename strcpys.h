#pragma once
// MS Visual C++ prefers to use strcpy_s instead of strcpy, which has 
// an extra parameter in the middle specifying the destination buffer 
// length. But this is not part of the C++ standard,  so for non-MSVC 
// compilers, we'll alias it to the standard function by dropping the 
// length parameter. There is a slight difference in behavior between 
// strcpy and strcpy_s with a source cstring shorter than the destin-
// ation  buffer,  but that  difference doesn't  matter for  what the 
// function is used for here.
#ifndef _MSC_VER
#define strcpy_s(d, l, s) strcpy(d, s)
#endif


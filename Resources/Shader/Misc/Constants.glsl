#ifndef MISC_CONSTANTS_GLSL
#define MISC_CONSTANTS_GLSL

const float kMinCosTheta = 1e-6f;

#define M_2PI 6.283185307179586476925286766559f
#define INV_M_PI (1.0f / M_PI)
#define INV_M_2PI (1.0f / M_2PI)

// Constants from <cmath>
#define M_E                 2.71828182845904523536  // e
#define M_LOG2E             1.44269504088896340736  // log2(e)
#define M_LOG10E            0.434294481903251827651 // log10(e)
#define M_LN2               0.693147180559945309417 // ln(2)
#define M_LN10              2.30258509299404568402  // ln(10)
#define M_PI                3.14159265358979323846  // pi
#define M_PI_2              1.57079632679489661923  // pi/2
#define M_PI_4              0.785398163397448309616 // pi/4
#define M_1_PI              0.318309886183790671538 // 1/pi
#define M_2_PI              0.636619772367581343076 // 2/pi
#define M_1_SQRTPI          0.564189583547756279280 // 1/sqrt(pi)
#define M_2_SQRTPI          1.12837916709551257390  // 2/sqrt(pi)
#define M_SQRT2             1.41421356237309504880  // sqrt(2)
#define M_SQRT1_2           0.707106781186547524401 // 1/sqrt(2)

// Numeric limits from <cfloat>
#define DBL_DECIMAL_DIG     17                      // # of decimal digits of rounding precision
#define DBL_DIG             15                      // # of decimal digits of precision
#define DBL_EPSILON         2.2204460492503131e-016 // smallest such that 1.0+DBL_EPSILON != 1.0
#define DBL_HAS_SUBNORM     1                       // type does support subnormal numbers
#define DBL_MANT_DIG        53                      // # of bits in mantissa
#define DBL_MAX             1.7976931348623158e+308 // max value
#define DBL_MAX_10_EXP      308                     // max decimal exponent
#define DBL_MAX_EXP         1024                    // max binary exponent
#define DBL_MIN             2.2250738585072014e-308 // min positive value
#define DBL_MIN_10_EXP      (-307)                  // min decimal exponent
#define DBL_MIN_EXP         (-1021)                 // min binary exponent
#define DBL_RADIX           2                       // exponent radix
#define DBL_TRUE_MIN        4.9406564584124654e-324 // min positive value

#define FLT_DECIMAL_DIG     9                       // # of decimal digits of rounding precision
#define FLT_DIG             6                       // # of decimal digits of precision
#define FLT_EPSILON         1.192092896e-07F        // smallest such that 1.0+FLT_EPSILON != 1.0
#define FLT_HAS_SUBNORM     1                       // type does support subnormal numbers
#define FLT_GUARD           0
#define FLT_MANT_DIG        24                      // # of bits in mantissa
#define FLT_MAX             3.402823466e+38F        // max value
#define FLT_MAX_10_EXP      38                      // max decimal exponent
#define FLT_MAX_EXP         128                     // max binary exponent
#define FLT_MIN             1.175494351e-38F        // min normalized positive value
#define FLT_MIN_10_EXP      (-37)                   // min decimal exponent
#define FLT_MIN_EXP         (-125)                  // min binary exponent
#define FLT_NORMALIZE       0
#define FLT_RADIX           2                       // exponent radix
#define FLT_TRUE_MIN        1.401298464e-45F        // min positive value

// Numeric limits from <cstdint>
#define UINT32_MAX          4294967295
#define INT32_MIN           -2147483648
#define INT32_MAX           2147483647

#define PiOver2 1.5707963267948966192313216916398f
#define PiOver4 0.78539816339744830961566084581988f
#define Sqrt2 1.4142135623730950488016887242097f

#endif
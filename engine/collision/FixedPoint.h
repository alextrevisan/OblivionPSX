#ifndef _FIXED_POINT_H_
#define _FIXED_POINT_H_
namespace ps1
{

    /* Multiply 2 signed 32 bit numbers for a 64 bit result and return the upper 32 bits */
/* long mul64u32(long a, long b) */
#define	mul64u32(a,b)	  			\
	({ long r0,r1=a,r2=b; 			\
	__asm__ volatile (	  			\
	"mult	%1, %2;"	  			\
	"mfhi	%0;"		  			\
	: "=r"( r0 )		  			\
	: "r"( r1 ), "r"( r2 )			\
	);					  			\
	r0; })

/* 
 */

/* Multiply 2 unsigned 32 bit numbers for a 64 bit result and return the upper 32 bits */
/* long mulu64u32(unsigned long a, unsigned long b) */
#define	mulu64u32(a,b)	  			\
	({ long r0,r1=a,r2=b; 			\
	__asm__ volatile (	  			\
	"multu	%1, %2;"	  			\
	"mfhi	%0;"		  			\
	: "=r"( r0 )		  			\
	: "r"( r1 ), "r"( r2 )			\
	);					  			\
	r0; })
/* void	start_SignedMultiply(long a, long b) */
#define	start_SignedMultiply(r0,r1)	\
	__asm__ volatile (				\
	"mult	%0, %1;"				\
	:								\
	: "r"( r0 ), "r"( r1 )			\
	);								\

/* long get_MultiplyHigh(void) */
#define	get_MultiplyHigh()			\
	({ long r0;			 			\
	__asm__ volatile (	  			\
	"mfhi	%0;"		  			\
	: "=r"( r0 )		  			\
	:								\
	);					  			\
	r0; })

/* long get_MultiplyLow(void) */
#define	get_MultiplyLow()			\
	({ long r0;			 			\
	__asm__ volatile (	  			\
	"mflo	%0;"		  			\
	: "=r"( r0 )		  			\
	:								\
	);					  			\
	r0; })

#define FixedMul(r0, r1, result)            \
    __asm__ volatile (                      \
    "mult %1, %2;"                          \
    "mflo $t0;"                             \
    "mfhi $t1;"                             \
    "srl $t2, $t0, 12;"                     \
    "sll $t3, $t1, 20;"                     \
    "or %0, $t2, $t3;"                      \
    : "=r"(result)                          \
    : "r"(r0), "r"(r1)                      \
    : "$t0", "$t1", "$t2", "$t3"            \
    )

/*
; s32 FixedMul(s32 a, s32 b)
	global	FixedMul
FixedMul

	mult    a0,a1
	mfhi    a3
	mflo    a2
	srl     v0,a2,12
	sll     a0,a3,20
	or      v0,v0,a0
	j      ra
	nop
*/
template <int FractionBits, typename StoreType, typename IntermediateType = long long, bool UseLibC = true>
struct FixedPoint
{
public:
    FixedPoint() : _value(0) {}
    FixedPoint(const FixedPoint &other) : _value(other._value) {}
    FixedPoint(const int& value) : _value(value << FractionBits) {}
    FixedPoint(const volatile int& value) : _value(value << FractionBits) {}
    FixedPoint(const unsigned int& value) : _value(value << FractionBits) {}
    FixedPoint(const long& value) : _value(value << FractionBits) {}
    FixedPoint(const unsigned long& value) : _value(value << FractionBits) {}
    FixedPoint(const short& value) : _value(value << FractionBits) {}
    FixedPoint(const unsigned short& value) : _value(value << FractionBits) {}
    FixedPoint(const float& value) : _value(value * _ONE) {}
    FixedPoint(const double& value) : _value(value * _ONE) {}

    explicit operator int() const                      { return AsInt(); }
    explicit operator long() const                     { return AsInt(); }
    explicit operator unsigned int() const             { return AsInt(); }
    explicit operator unsigned long() const            { return AsInt(); }
    explicit operator float() const                    { return AsFloat(); }
    explicit operator double() const                   { return AsFloat(); }

    FixedPoint &operator=(const FixedPoint &other) { _value = other._value; return *this; }

    constexpr bool operator==(const FixedPoint &other) const { return _value == other._value; }
    constexpr bool operator!=(const FixedPoint &other) const { return _value != other._value; }
    constexpr bool operator<(const FixedPoint &other) const { return _value < other._value; }
    constexpr bool operator<=(const FixedPoint &other) const { return _value <= other._value; }
    constexpr bool operator>(const FixedPoint &other) const { return _value > other._value; }
    constexpr bool operator>=(const FixedPoint &other) const { return _value >= other._value; }

    constexpr FixedPoint operator~() const { return set(~_value); }
    constexpr FixedPoint operator+() const { return *this; }
    constexpr FixedPoint operator-() const { return set(-_value); }

    constexpr inline FixedPoint operator+(const FixedPoint &other) const { return set(_value + other._value); }
    constexpr inline FixedPoint operator-(const FixedPoint &other) const { return set(_value - other._value); }
    constexpr inline FixedPoint operator*(const FixedPoint &other) const { 
            StoreType out;
            FixedMul(_value,other._value, out);
            return set(out);
        }
    inline constexpr FixedPoint operator/(const FixedPoint &other) const
    {
        if constexpr (UseLibC)
        {
            if(other._value == 0) return 0;
            return set((((IntermediateType)_value)<<FractionBits)/other._value);
        }
        else
        {
            if(other._value == 0) return 0;
            return set(Divide(_value,other._value));
        }
    }

    FixedPoint& operator+=(const FixedPoint &other) { _value += other._value; return *this; }
    FixedPoint& operator-=(const FixedPoint &other) { _value -= other._value; return *this; }
    FixedPoint& operator*=(const FixedPoint &other) { _value = ((IntermediateType)_value * (IntermediateType)other._value) >> FractionBits; return *this; }
    FixedPoint& operator/=(const FixedPoint &other) { _value = ((((IntermediateType) _value)<<FractionBits) / other._value); return *this; }
    FixedPoint& operator%=(const long& other) { _value = (((IntermediateType)_value)<<FractionBits) % other;  return *this; }

    FixedPoint& operator|=(const FixedPoint& other) { _value |=  other._value; return *this;}
    FixedPoint& operator&=(const FixedPoint& other) { _value &= other._value; return *this;}
    FixedPoint& operator^=(const FixedPoint& other) { _value ^= other._value; return *this;}

    FixedPoint operator<<(int numBits) const { return set(_value << numBits); }
    FixedPoint operator>>(int numBits) const { return set(_value >> numBits); }

    FixedPoint& operator<<=(int numBits) { _value <<= numBits; return *this;}
    FixedPoint& operator>>=(int numBits) { _value >>= numBits; return *this;}

    FixedPoint& operator++() { _value += 1<<FractionBits; return *this; } 
    FixedPoint& operator--() { _value -= 1<<FractionBits; return *this; }
    FixedPoint& operator++(int) { _value += 1<<FractionBits; return *this; } 
    FixedPoint& operator--(int) { _value -= 1<<FractionBits; return *this; }

    constexpr inline FixedPoint Abs() const
    { 
        int32_t temp = _value >> 31;
        int32_t value = _value;
        value ^= temp;
        value -= temp;
        return set(value); 
    }
    
    static constexpr inline FixedPoint Sin(const FixedPoint& angle)
    {
        return FixedPoint::FromFixedPoint(isin(angle._value));
    }

    static constexpr inline FixedPoint Cos(const FixedPoint& angle)
    {
        return FixedPoint::FromFixedPoint(icos(angle._value));
    }

    static constexpr inline FixedPoint Sqrt(const FixedPoint& value)
    {
        return FixedPoint::FromFixedPoint(SquareRoot12(value._value));
    }

    static FixedPoint Acos(const FixedPoint& value)
    {
        // Ensure the input is clamped between -1 and 1
        auto x = Clamp(value, FixedPoint(-1), FixedPoint(1));

        // Polynomial coefficients for Chebyshev approximation of arccos(x)
        const FixedPoint c1 = 1.5707288;
        const FixedPoint c2 = -0.2121144;
        const FixedPoint c3 = 0.0742610;
        const FixedPoint c4 = -0.0187293;

        FixedPoint sqrtValue = Sqrt(FixedPoint(1) - x * x);
        FixedPoint result = c1 + x * (c2 + x * (c3 + x * c4));
        result = result * sqrtValue;

        return result;
    }
    
    static FixedPoint Clamp(FixedPoint value, FixedPoint min, FixedPoint max)
    {
        if (value < min)
            return min;
        if (value > max)
            return max;
        return value;
    }

    constexpr StoreType AsFixedPoint() const { return _value; }
    static constexpr inline FixedPoint FromFixedPoint(StoreType value) { return set(value); }

    constexpr int AsInt() const { return _value >> FractionBits; }
    constexpr float AsFloat() const { return (float)_value / _ONE; }

    static constexpr FixedPoint PI() { return set(_ONE * 3.14159265359f); }
    static constexpr FixedPoint E() { return set(_ONE * 2.71828182845905); }
    static constexpr FixedPoint Zero() { return set(0); }
    static constexpr FixedPoint One() { return set(_ONE); }
    static constexpr FixedPoint MinValue(){ return set(2147483646); }
    static constexpr FixedPoint MaxValue(){ return set(2147483647); }
    static constexpr FixedPoint Epsilon() { return 0.000244140625; }

private:
    StoreType _value;
    static constexpr StoreType _ONE = 1 << FractionBits;
    static constexpr int INTEGER_BITS = sizeof(int) * 8 - FractionBits;
    static constexpr int FRACTION_MASK = (((int)0xFFFFFFFF) >> FractionBits);
    static constexpr int INTEGER_MASK = (-1 & ~FRACTION_MASK);
    static constexpr int FRACTION_RANGE = FRACTION_MASK + 1;
    static constexpr int MIN_INTEGER = (-2147483647 - 1) >> FractionBits;
    static constexpr int MAX_INTEGER = (2147483647) >> FractionBits;

    static constexpr inline FixedPoint set(StoreType value)
    {
        FixedPoint f;
        f._value = value;
        return f;
    }
    inline const unsigned int internalDivision(unsigned long long rem, unsigned int base) const
    {
        rem <<= FractionBits;
        unsigned long long b = base;
        unsigned long long res, d = 1;
        unsigned int high = rem >> 32;

        res = 0;
        if (high >= base) {
            high /= base;
            res = (unsigned long long)high << 32;
            rem -= (unsigned long long)(high * base) << 32;
        }

        while ((long long)b > 0 && b < rem) {
            b = b + b;
            d = d + d;
        }

        do {
            if (rem >= b) {
                rem -= b;
                res += d;
            }
            b >>= 1;
            d >>= 1;
        } while (d);

        return res;
    }

    inline constexpr int Divide(int a, int b) const
    {
        int s = 1;
        if (a < 0) {
            a = -a;
            s = -1;
        }
        if (b < 0) {
            b = -b;
            s = -s;
        }
        return internalDivision(a, b) * s;
    }

}; //struct FixedPoint

} //namespace ps1

typedef ps1::FixedPoint<12, int> FixedPoint;

#endif //_FIXED_POINT_H_

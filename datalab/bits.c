/*
 * CS:APP Data Lab
 *
 * Miu Nakajima <mnakajim>
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 */

/* Instructions to Students:

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:

  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code
  must conform to the following style:

  long Funct(long arg1, long arg2, ...) {
      // brief description of how your implementation works
      long var1 = Expr1;
      ...
      long varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. (Long) integer constants 0 through 255 (0xFFL), inclusive. You are
      not allowed to use big constants such as 0xffffffffL.
  2. Function arguments and local variables (no global variables).
  3. Local variables of type int and long
  4. Unary integer operations ! ~
     - Their arguments can have types int or long
     - Note that ! always returns int, even if the argument is long
  5. Binary integer operations & ^ | + << >>
     - Their arguments can have types int or long
  6. Casting from int to long and from long to int

  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting other than between int and long.
  7. Use any data type other than int or long.  This implies that you
     cannot use arrays, structs, or unions.

  You may assume that your machine:
  1. Uses 2s complement representations for int and long.
  2. Data type int is 32 bits, long is 64.
  3. Performs right shifts arithmetically.
  4. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31 (int) or 63 (long)

EXAMPLES OF ACCEPTABLE CODING STYLE:
  //
  // pow2plus1 - returns 2^x + 1, where 0 <= x <= 63
  //
  long pow2plus1(long x) {
     // exploit ability of shifts to compute powers of 2
     // Note that the 'L' indicates a long constant
     return (1L << x) + 1L;
  }

  //
  // pow2plus4 - returns 2^x + 4, where 0 <= x <= 63
  //
  long pow2plus4(long x) {
     // exploit ability of shifts to compute powers of 2
     long result = (1L << x);
     result += 4L;
     return result;
  }

NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

CAUTION:
  Do not add an #include of <stdio.h> (or any other C library header)
  to this file.  C library headers almost always contain constructs
  that dlc does not understand.  For debugging, you can use printf,
  which is declared for you just below.  It is normally bad practice
  to declare C library functions by hand, but in this case it's less
  trouble than any alternative.

  dlc will consider each call to printf to be a violation of the
  coding style (function calls, after all, are not allowed) so you
  must remove all your debugging printf's again before submitting your
  code or testing it with dlc or the BDD checker.  */

extern int printf(const char *, ...);

/* Edit the functions below.  Good luck!  */
// 2
/*
 * implication - given input x and y, which are binary
 * (taking  the values 0 or 1), return x -> y in propositional logic -
 * 0 for false, 1 for true
 *
 * Below is a truth table for x -> y where A is the result
 *
 * |-----------|-----|
 * |  x  |  y  |  A  |
 * |-----------|-----|
 * |  1  |  1  |  1  |
 * |-----------|-----|
 * |  1  |  0  |  0  |
 * |-----------|-----|
 * |  0  |  1  |  1  |
 * |-----------|-----|
 * |  0  |  0  |  1  |
 * |-----------|-----|
 *
 *
 *   Example: implication(1L,1L) = 1L
 *            implication(1L,0L) = 0L
 *   Legal ops: ! ~ ^ |
 *   Max ops: 5
 *   Rating: 2
 */
long implication(long x, long y) {
    return ((!(x ^ y)) | y);
}
/*
 * leastBitPos - return a mask that marks the position of the
 *               least significant 1 bit. If x == 0, return 0
 *   Example: leastBitPos(96L) = 0x20L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
long leastBitPos(long x) {
    long y = ~x; // inverts x
    y += 1;      // adding 0x1 carries over until the bit is zero
    return (x & y);
}
/*
 * distinctNegation - returns 1 if x != -x.
 *     and 0 otherwise
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 5
 *   Rating: 2
 */
long distinctNegation(long x) {
    return !!(x + x);
}
/*
 * fitsBits - return 1 if x can be represented as an
 *  n-bit, two's complement integer.
 *   1 <= n <= 64
 *   Examples: fitsBits(5,3) = 0L, fitsBits(-4,3) = 1L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
long fitsBits(long x, long n) {
    long a = x >> 63;         // most significant bit
    long b = x >> (n + (~0)); // shift x to the right by n-1 bits
    return !(a ^ b);
}
// 3
/*
 * trueFiveEighths - multiplies by 5/8 rounding toward 0,
 *  avoiding errors due to overflow
 *  Examples:
 *    trueFiveEighths(11L) = 6L
 *    trueFiveEighths(-9L) = -5L
 *    trueFiveEighths(0x3000000000000000L) = 0x1E00000000000000L (no overflow)
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 20
 *  Rating: 4
 */
long trueFiveEighths(long x) {
    long sign = x >> 63;               // sign bit
    long msk_x = x & 7;                // saves last 3 bits
    msk_x = (msk_x << 2) + msk_x;      // multiplies by 5
    msk_x = (msk_x + (7 & sign)) >> 3; // divides by 8
    x = x >> 3;                        // divides by 8
    x = (x << 2) + x;                  // multiplies by 5
    x = x + msk_x;                     // adds the saved bits
    return x;
}
/*
 * addOK - Determine if can compute x+y without overflow
 *   Example: addOK(0x8000000000000000L,0x8000000000000000L) = 0L,
 *            addOK(0x8000000000000000L,0x7000000000000000L) = 1L,
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
long addOK(long x, long y) {
    long x_sign = x >> 63; // most significant bit
    long y_sign = y >> 63;
    long sum_sign = (x + y) >> 63;

    long a = x_sign ^ y_sign;
    long b = x_sign ^ sum_sign;
    long c = y_sign ^ sum_sign;

    return (!!a) | (!(b & c));
}
/*
 * isPower2 - returns 1 if x is a power of 2, and 0 otherwise
 *   Examples: isPower2(5L) = 0L, isPower2(8L) = 1L, isPower2(0) = 0L
 *   Note that no negative number is a power of 2.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
long isPower2(long x) {
    long y = x + (~0);  // digits change with powers of 2
    long tmp = x >> 63; // most significant bit of x (excludes int_min)
    long msk = y >> 63; // most significant bit of y (excludes 0)
    return !((x & y) | (tmp | msk));
}
/*
 * rotateLeft - Rotate x to the left by n
 *   Can assume that 0 <= n <= 63
 *   Examples:
 *      rotateLeft(0x8765432187654321L,4L) = 0x7654321876543218L
 *   Legal ops: ~ & ^ | + << >> !
 *   Max ops: 25
 *   Rating: 3
 */
long rotateLeft(long x, long n) {
    long shift = (65 + (~n)); // 64 - n
    long neg1 = ~0;
    long msk = neg1 << shift;      // mask for the first n bits
    long msk2 = ~(neg1 << n);      // mask for the last n bits
    long tmp = (x & msk) >> shift; // mask on x and shifts to rotate
    x = x << n;                    // shifts x over by n
    return (x | (tmp & msk2));
}
// 4
/*
 * isPalindrome - Return 1 if bit pattern in x is equal to its mirror image
 *   Example: isPalindrome(0x6F0F0123c480F0F6L) = 1L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 70
 *   Rating: 4
 */
long isPalindrome(long x) {
    long y = x;               // keeps a copy of x
    long msk = ~(~0L << 32);  // creates a half mask
    long r = (x & msk) << 32; // takes right half of x
    long l = (x >> 32) & msk; // takes left half of x
    x = r | l;                // combines rotated halves
    long tmp = msk >> 16;     // repeats for each sect
    msk = (tmp << 32) | tmp;
    r = (x & msk) << 16;
    l = (x >> 16) & msk;
    x = r | l;
    tmp = msk >> 40;
    msk = (tmp) | (tmp << 16) | (tmp << 32) | (tmp << 48);
    r = (x & msk) << 8;
    l = (x >> 8) & msk;
    x = r | l;
    tmp = msk >> 52;
    msk = (tmp) | (tmp << 8) | (tmp << 16) | (tmp << 24) | (tmp << 32) |
          (tmp << 40) | (tmp << 48) | (tmp << 56);
    r = (x & msk) << 4;
    l = (x >> 4) & msk;
    x = r | l;
    tmp = msk >> 58;
    msk = (tmp) | (tmp << 4) | (tmp << 8) | (tmp << 12) | (tmp << 16) |
          (tmp << 20) | (tmp << 24) | (tmp << 28) | (tmp << 32) | (tmp << 36) |
          (tmp << 40) | (tmp << 44) | (tmp << 48) | (tmp << 52) | (tmp << 56) |
          (tmp << 60);
    r = (x & msk) << 2;
    l = (x >> 2) & msk;
    x = r | l;
    tmp = msk >> 61;
    msk = (tmp) | (tmp << 2) | (tmp << 4) | (tmp << 6) | (tmp << 8) |
          (tmp << 10) | (tmp << 12) | (tmp << 14) | (tmp << 16) | (tmp << 18) |
          (tmp << 20) | (tmp << 22) | (tmp << 24) | (tmp << 26) | (tmp << 28) |
          (tmp << 30) | (tmp << 32) | (tmp << 34) | (tmp << 36) | (tmp << 38) |
          (tmp << 40) | (tmp << 42) | (tmp << 44) | (tmp << 46) | (tmp << 48) |
          (tmp << 50) | (tmp << 52) | (tmp << 54) | (tmp << 56) | (tmp << 58) |
          (tmp << 60) | (tmp << 62);
    r = (x & msk) << 1;
    l = (x >> 1) & msk;
    x = r | l;
    return !(x ^ y);
}
/*
 * bitParity - returns 1 if x contains an odd number of 0's
 *   Examples: bitParity(5L) = 0L, bitParity(7L) = 1L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 22
 *   Rating: 4
 */
long bitParity(long x) {
    x = x ^ (x >> 32); // checks number of 0's
    x = x ^ (x >> 16); // imitates a loop
    x = x ^ (x >> 8);
    x = x ^ (x >> 4);
    x = x ^ (x >> 2);
    x = x ^ (x >> 1);
    return (x & 1);
}
/*
 * absVal - absolute value of x
 *   Example: absVal(-1) = 1.
 *   You may assume -TMax <= x <= TMax
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 4
 */
long absVal(long x) {
    long temp = x >> 63; // -1 if negative, 0 if positive
    x = x ^ temp;        // makes x positive
    x = x + (temp & 1);  // adds 1 if x was negative
    return x;
}

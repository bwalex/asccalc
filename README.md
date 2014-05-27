asccalc - A Simple Console Calculator

Pretty much a fully featured console calculator. One of the few that makes
it easy to use logic operations and different bases.


Functionality:
==============

Binary Operators
---------
```
+        Addition
-        Subtraction
*        Multiplication
/        Divison
%        Modulo (Remainder)
**       raise to the power of...
^^       same as **
&        Logical And
and      same as &
|        Logical Or
or       same as |
^        Logical Exclusive-Or
xor      same as ^
<<       Logical shift left (arithmetic shift for negative numbers)
>>       Logical shift right (arithmetic shift for negative numbers)
```


Numbers
----------
Numbers can be expressed in many different ways. Here are some examples:

```
0xFF
0b010101
5
0d5
0755
5.71
5.71E-5
5.71p
```

As the last example shows, several SI suffixes are available, with their
usual meaning:

    a,f,p,n,u,m,k,M,G,T,P,E



Comparison Operators (return 1 if true, otherwise 0)
----------
```
>=       Greater or equal
<=       Less or equal
!=       Not equal
==       Equal
>        Greater than
<        Less than
```




Unary Operators
---------
```
-          Unary minus
~          1's complement
!          Factorial
[hi:lo]    Verilog-style part select (select bits from MSB=hi to LSB=lo)
[hi-:cnt]  Verilog-style part select (select cnt bits downwards from MSB=hi)
```




Statements
---------
A statement (i.e. an expression using operators) is evaluated at the
end of the line (e.g. when ENTER is pressed) unless the last character
of the line was a backslash ('\'), in which case the newline is ignored.
Statements can have a trailing semicolon (';') when stand-alone, and must
have a trailing semicolon (';') when they are part of a listing (more on
that later).
Example:

    1 + 5 * 7

or

    1 + 5 * 7;

or

    1 + 5 * \
    7

or

    1 + 5 * \
    7;


User-defined variables
--------
User-defined variables can be created with the following syntax,

    VARIABLE_NAME = STATEMENT

For example,

    x = 5 * pi;

where the trailing semicolon (';') is optional.




Flow control
--------
if/elsif/else and while flow control is available.

The conditional construct if is as follows, where the 'else LISTING' part
is optional,

    if CONDITION then LISTING else LISTING fi

A conditional construct can also contain any number of elsifs:

    if CONDITION then LISTING elsif CONDITION then LISTING else LISTING fi

    if CONDITION then LISTING elsif CONDITION then LISTING elsif CONDITION then LISTING else LISTING fi

For incomplete if statements (i.e. lacking else) a default value of 0 is
returned if no condition matches. Otherwise the value of the last expression
to be evaluated in a LISTING is returned.
The condition can be any statement (see above) that is considered true
if it evaluates to a non-zero value. The LISTING is a series of statements
delimited by a trailing semicolon (';') or other flow control constructs.
After the 'if', a newline can be introduced at any time until the closing
'fi'.
Examples:

    if (x > 5) then
     x/5;
    else
     x*5;
    fi

    if (x > 5)
    then
     x/5;
    fi

The while construct similarly is,

    while CONDITION do LISTING done

A default value of 0 is returned if the condition is false. Otherwise the
value of the last evaluated expression is returned.
The condition can be any statement (see above) that is considered
true if it evaluates to a non-zero value. The LISTING is a series of
statements delimited by a trailing semicolon (';') or other flow control
constructs.
After the 'while', a newline can be introduced at any time until the
closing 'done'.
Examples:

    x = 1;
    while (x < 5) do
     x = x * 2;
    done




User-defined functions
--------
User-defined functions can be defined using the function keyword. The
syntax is as follows:

    function NAME(NAMELIST) = LISTING endfunction

The LISTING is a series of statements delimited by a trailing
semicolon (';') or flow control constructs.
The NAMELIST is a list of comma-separated argument variables to the
function.
After the 'function' token, a newline can be introduced at any time
until the closing 'endfunction'.
All user-defined variables assigned in the body of the function are
local to the function.
The scope inside a function is the global scope and the direct
arguments to the function. It does not include any other scope, e.g.
the scope of the calling function for a nested function.

Example:

    function sum(a,b) =
     a + b;
    endfunction

    function foobar(a,b,c) =
     x = e * a;
     x = sum(x, a);
     root(x, c);
    endfunction




Comments
---------
Comments can either start with // or #




Keywords (i.e. reserved words)
---------
if, then, else, fi, while, do, done, function, endfunction, ls, lsfn,
quit, exit, help, mode, and, or, xor




Commands
---------
```
ls           Lists all variables
lsfn         Lists all functions (builtin and user-defined)
m <mode>     Same as mode <mode>
mode <mode>  Switches to output mode <MODE>, where mode is one of
             the following:
               b - for binary output
               d - for decimal output
	       s - for decimal scientific output
               h or x - for hexadecimal output
               o - for octal output
quit         Exits the program
exit         Exits the program
```




Builtin functions
---------
Check with the 'lsfn' commands for the most up-to-date list.

```
sqrt(a)       Square Root of a
cbrt(a)       Cubic Root of a
root(a,n)     n-th root of a
abs(a)        Absolute value of a
ln(a)         Natural Logarithm of a
log2(a)       Logarithm base 2 of a
log10(a)      Logarithm base 10 of a
exp(a)        exponential of a
sec(a)
csc(a)
cot(a)
cos(a)
sin(a)
tan(a)
acos(a)
asin(a)
atan(a)
atan2(y,x)
cosh(a)
sinh(a)
tanh(a)
sech(a)
csch(a)
coth(a)
acosh(a)
asinh(a)
atanh(a)
erf(a)        Error function of a
erfc(a)       Complementary error function of a
hypot(x,y)    Hypotenuse of x and y, i.e. sqrt(x^2 + y^2)
round(a)      Round a to nearest integer
ceil(a)       Round a upwards to nearest integer
floor(a)      Round a downwards to nearest integer
trunc(a)      Truncate a to an integer
int(a)        Same as trunc(a)
nextprime(a)  Gives the next highest prime number after a
gcd(a,b)      Greatest common divisor of a and b
lcm(a,b)      Least common multiple of a and b
remfac(a,b)   Remove factor b from a
bin(a,b)      Binomial coefficient (a | b)
fib(n)        n-th fibonacci number
inv(a,N)      Find the inverse of a (modulo N)
hamdist(a,b)  Gives the hamming distance between integers a and b
countones(a)  Returns the number of 1-bits in integer a
popcount(a)   Same as countones(a)
popcnt(a)     Same as countones(a)
min(a,b,...)  Minimum of a,b,...
max(a,b,...)  Maximum of a,b,...
avg(a,b,...)  Average of a,b,...
```





Build and Install
=============

Dependencies
-------------
```
flex (build only)
bison (build only)
libmpfr
libgmp
```

To build on Ubuntu and Debian:

    apt-get install flex bison libmpfr-dev libgmp-dev make gcc


To just use a binary package:

    apt-get install libgmp10 libmpfr4


Build
------------
Just type

    make


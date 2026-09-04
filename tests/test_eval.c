/*
 * Evaluation regressions: crashes, precision, precedence, and the
 * 'digits' command. Every case here once crashed or gave a wrong answer.
 */
#include "harness.h"

int
main(void)
{
	/* --- division by zero used to raise SIGFPE inside GMP --- */
	expect_output("1/0", "inf\n");
	expect_output("-1/0", "-inf\n");
	expect_output("7/0.0", "inf\n");
	expect_output("1/0.5", "2\n");
	expect_error("5 % 0");
	expect_error("5.5 % 0");
	expect_error("remfac(5, 0)");
	expect_error("invert(2, 0)");
	expect_error("invert(2, 4)");
	expect_output("invert(3, 11)", "4\n");
	expect_output("remfac(40, 2)", "5\n");

	/* --- part select --- */
	expect_error("5[0:3]");
	expect_output("0xAB[7:4]", "10\n");
	expect_output("0xAB[3-:4]", "11\n");
	expect_output("0xAB[0]", "1\n");
	expect_output("(-8)[7:4]", "15\n");
	expect_output("(-8) >> 4", "-1\n");

	/* --- missing or failing arguments used to segfault --- */
	expect_error("sgn(foo)");
	expect_error("gcd(foo, 4)");
	expect_error("sqrt(foo)");
	expect_error("fib(-1)");
	expect_error("bin(5, -1)");
	expect_error("root(2, -1)");
	expect_contains("root(2, -1)", "'root'");
	expect_error("tabulate(sqrt, foo)");

	/* --- loops --- */
	expect_error("while foo do 1; done");
	expect_output("while 0 do done", "0\n");
	expect_output("i = 0\nwhile i < 5 do i = i + 1; done\ni", "0\n5\n5\n");
	expect_output("i = 0\nwhile i < 20000 do i = i + 1; done", "0\n20000\n");

	/* --- require --- */
	expect_contains("require \"\"\n3", "3\n");
	expect_contains("require \"/nonexistent/file\"\n3", "3\n");

	/* --- user functions --- */
	/* returning a local used to return freed memory */
	expect_output("function f(a) = x = a*2; x; endfunction\nf(3)\nf(3) + 1",
	    "Defined function 'f'\n6\n7\n");
	expect_output("function f(a) = x = a*2; endfunction\nf(3)",
	    "Defined function 'f'\n6\n");
	expect_output("function f(x) = x = x + 1; x; endfunction\nf(1)\nf(1)",
	    "Defined function 'f'\n2\n2\n");
	expect_output(
	    "function fact(n) = if n <= 1 then 1; else n * fact(n-1); fi endfunction\nfact(20)",
	    "Defined function 'fact'\n2432902008176640000\n");
	/* locals do not leak out; globals are readable */
	expect_output("g = 10\nfunction h(x) = g = x; g; endfunction\nh(3)\ng",
	    "10\nDefined function 'h'\n3\n10\n");
	expect_output("g = 10\nfunction h(x) = g + x; endfunction\nh(3)",
	    "10\nDefined function 'h'\n13\n");
	/* redefinition */
	expect_output(
	    "function f(x) = x*2; endfunction\nf(2)\nfunction f(x) = x*3; endfunction\nf(2)",
	    "Defined function 'f'\n4\nDefined function 'f'\n6\n");
	/* two-argument function whose argument names collide in the table */
	expect_output("function s(a, b, c, d) = a + b + c + d; endfunction\ns(1, 2, 3, 4)",
	    "Defined function 's'\n10\n");

	/* --- recursion depth is bounded, not a stack overflow --- */
	expect_error("function f(n) = f(n+1); endfunction\nf(0)");
	expect_contains("function f(n) = f(n+1); endfunction\nf(0)", "call depth");
	expect_output(
	    "function d(n) = if n <= 0 then 0; else d(n-1) + 1; fi endfunction\nd(1500)",
	    "Defined function 'd'\n1500\n");
	/* the calculator keeps working after the limit was hit */
	expect_contains("function f(n) = f(n+1); endfunction\nf(0)\n1 + 1", "\n2\n");

	/* --- leading zeros in decimal literals --- */
	expect_output("08", "8\n");
	expect_output("09 + 1", "10\n");
	expect_output("0d08", "8\n");
	expect_output("010", "8\n");
	expect_output("0755", "493\n");
	expect_output("0xFF + 0b11", "258\n");
	expect_output("08.5", "8.5\n");

	/* --- syntax errors recover and discard partial parses cleanly --- */
	expect_output("1 + + 2\n3", "<stdin>:1: error: syntax error\n3\n");
	/* an error found at end of line is reported with the next line number */
	expect_output("foo(1, 2,\n4", "<stdin>:2: error: syntax error\n4\n");
	/* inside a block the newline is not a terminator, so line 2 is eaten */
	expect_output("function f(a, b\n5\n6", "<stdin>:2: error: syntax error\n6\n");
	expect_output("x = 5 +\nx = 6\nx", "<stdin>:2: error: syntax error\n6\n6\n");
	expect_output("if 1 then 2; fi fi\n7", "<stdin>:1: error: syntax error\n7\n");

	/* --- variable aliasing --- */
	expect_output("x = 5\nx = x\nx", "5\n5\n5\n");
	expect_output("x = 5\ny = x + (x = 6)\ny\nx", "5\n11\n11\n6\n");

	/* --- precision --- */
	expect_output("1000! + 0 == 1000!", "1\n");
	expect_output("1000! - 1000!", "0\n");
	expect_output("3**2000 % 3", "0\n");
	expect_output("7**1000 % 7", "0\n");
	expect_output("2**-1", "0.5\n");
	expect_output("4**0.5", "2\n");
	expect_output("(-2)**3", "-8\n");

	/* --- SI suffixes parse exactly --- */
	expect_output("1m == 0.001", "1\n");
	expect_output("1u == 1e-6", "1\n");
	expect_output("5.71p == 5.71e-12", "1\n");
	expect_output("1.5E-3k == 1.5", "1\n");
	expect_output("2E == 2e18", "1\n");
	expect_output("3k", "3000\n");
	expect_output("5k & 1", "0\n");

	/* --- precedence: shifts bind like * and / (Go, Pascal) --- */
	expect_output("1 + 2 << 3", "17\n");
	expect_output("1 << 2 + 1", "5\n");
	expect_output("6 / 2 << 1", "6\n");
	expect_output("12 >> 2 / 2", "1.5\n");
	expect_output("2 * 3 << 1", "12\n");
	expect_output("1 << 4 - 1", "15\n");
	expect_output("0xF0 & 1 << 4", "0\n");
	expect_output("0xF0 & (1 << 4)", "16\n");
	expect_output("-2**2", "-4\n");
	expect_output("2**3!", "64\n");

	/* --- digits --- */
	expect_output("1/3", "0.333333\n");
	expect_output("digits 12\n1/3", "0.333333333333\n");
	expect_output("digits 12\n1234567.5", "1234567.5\n");
	expect_output("digits 3\nsqrt(2)", "1.41\n");
	expect_output("digits 12\nmode s\n1/3000000", "3.33333333333E-07\n");
	expect_output("mode s\n123456789.5", "1.23457E+08\n");
	expect_output("digits 12\ntabulate(sqrt, 2)", "2 | 1.414213562373\n");
	expect_error("digits 0");
	expect_error("digits 100000");
	expect_output("digits 9\ndigits 4\n1/3", "0.3333\n");

	/* --- ans stays an exact integer --- */
	expect_output("5\nans + 1", "5\n6\n");
	expect_output("2**100\nans == 2**100", "1267650600228229401496703205376\n1\n");
	expect_contains("1000!\nans == 1000!", "\n1\n");
	expect_output("10/4\nans * 2", "2.5\n5\n");

	return finish("eval");
}

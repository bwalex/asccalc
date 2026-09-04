#include "harness.h"

int
main(void)
{
	/* builtin: signature, summary, argument table, returns, example */
	expect_contains("help root", "root(a, n) [builtin]");
	expect_contains("help root", "n-th root.");
	expect_contains("help root", "Degree of the root.");
	expect_contains("help root", "root(81, 4) => 3");

	/* variadic builtin shows the ellipsis */
	expect_contains("help min", "min(a, b, ...) [builtin]");

	/* user-defined: names come from the definition */
	expect_contains(
	    "function area(w, h) = w * h; endfunction\nhelp area",
	    "area(w, h) [user-defined]");
	expect_contains(
	    "function area(w, h) = w * h; endfunction\nhelp area",
	    "  w  User-defined parameter.");

	/* unknown name is an error, not a crash */
	expect_error("help nosuchfunction");

	/* trailing whitespace on commands is tolerated */
	expect_contains("help sqrt   ", "sqrt(a) [builtin]");
	expect_contains("help  ", "Commands:");
	expect_contains("help", "Operator precedence");

	return finish("function help");
}

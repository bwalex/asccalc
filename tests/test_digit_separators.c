#include "harness.h"

struct valid_case {
	const char *expr;
	const char *expected;
};

int
main(void)
{
	static const struct valid_case valid_cases[] = {

		{ "0", "0\n" },
		{ "1_000 + 2_000", "3000\n" },
		{ "0xFF_FF", "65535\n" },
		{ "0b1010_0001", "161\n" },
		{ "07_55", "493\n" },
		{ "1_234.5_6e1_0", "12345600000000\n" },
		{ "0d12_345", "12345\n" },
		{ "5.7_1E-5", "5.71e-05\n" },
		{ "0d1_000.0_5", "1000.05\n" },
		{ "5.7_1p", "5.71e-12\n" },
	};
	static const char *invalid_cases[] = {
		"1_",
		"1__0",
		"0b_1010",
		"0x_FF",
		"0xDEAD_",
		"123_.45",
		"123._45",
		"1e_9",
	};
	size_t i;

	for (i = 0; i < sizeof(valid_cases) / sizeof(valid_cases[0]); ++i)
		expect_output(valid_cases[i].expr, valid_cases[i].expected);

	for (i = 0; i < sizeof(invalid_cases) / sizeof(invalid_cases[0]); ++i)
		expect_error(invalid_cases[i]);

	return finish("digit separator");
}

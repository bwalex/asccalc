#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

struct valid_case {
	const char *expr;
	const char *expected;
};

static int failures;

static void
fatal(const char *msg)
{
	perror(msg);
	exit(1);
}

static char *
read_file(const char *path)
{
	FILE *fp;
	long len;
	size_t nread;
	char *buf;

	if ((fp = fopen(path, "rb")) == NULL)
		fatal("fopen");
	if (fseek(fp, 0, SEEK_END) != 0)
		fatal("fseek");
	len = ftell(fp);
	if (len < 0)
		fatal("ftell");
	if (fseek(fp, 0, SEEK_SET) != 0)
		fatal("fseek");

	if ((buf = malloc((size_t)len + 1)) == NULL)
		fatal("malloc");
	if ((nread = fread(buf, 1, (size_t)len, fp)) != (size_t)len) {
		if (ferror(fp))
			fatal("fread");
	}
	buf[nread] = '\0';

	if (fclose(fp) != 0)
		fatal("fclose");

	return buf;
}

static char *
run_expr(const char *expr)
{
	char in_template[] = "/tmp/asccalc-in-XXXXXX";
	char out_template[] = "/tmp/asccalc-out-XXXXXX";
	char cmd[512];
	FILE *fp;
	int in_fd, out_fd, status;
	char *output;

	if ((in_fd = mkstemp(in_template)) < 0)
		fatal("mkstemp");
	if ((out_fd = mkstemp(out_template)) < 0)
		fatal("mkstemp");

	if ((fp = fdopen(in_fd, "w")) == NULL)
		fatal("fdopen");
	if (fprintf(fp, "%s\n", expr) < 0)
		fatal("fprintf");
	if (fclose(fp) != 0)
		fatal("fclose");
	if (close(out_fd) != 0)
		fatal("close");

	if (snprintf(cmd, sizeof(cmd), "./asccalc < '%s' > '%s' 2>&1", in_template, out_template) >= (int)sizeof(cmd)) {
		fprintf(stderr, "command buffer too small\n");
		exit(1);
	}

	status = system(cmd);
	if (status == -1)
		fatal("system");
	if (WIFSIGNALED(status)) {
		fprintf(stderr, "asccalc terminated by signal %d\n", WTERMSIG(status));
		exit(1);
	}

	output = read_file(out_template);
	unlink(in_template);
	unlink(out_template);
	return output;
}

static void
expect_output(const char *expr, const char *expected)
{
	char *output;

	output = run_expr(expr);
	if (strcmp(output, expected) != 0) {
		fprintf(stderr,
		    "FAIL: %s\nexpected: %sactual:   %s\n",
		    expr, expected, output);
		failures++;
	}
	free(output);
}

static void
expect_error(const char *expr)
{
	char *output;

	output = run_expr(expr);
	if (strstr(output, "error") == NULL) {
		fprintf(stderr, "FAIL: %s\nexpected an error, got: %s\n", expr, output);
		failures++;
	}
	free(output);
}

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

	if (failures != 0) {
		fprintf(stderr, "%d test(s) failed\n", failures);
		return 1;
	}

	printf("digit separator tests passed\n");
	return 0;
}

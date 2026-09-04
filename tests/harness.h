/*
 * Shared helpers for the asccalc end-to-end tests.
 *
 * Each test feeds a script to ./asccalc on stdin (so run from the repo
 * root) with HOME pointed at an empty scratch directory, so the user's
 * ~/.asccalc.rc and rc.d cannot influence the results.
 */
#ifndef ASCCALC_TEST_HARNESS_H
#define ASCCALC_TEST_HARNESS_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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

static const char *
scratch_home(void)
{
	static char home[] = "/tmp/asccalc-home-XXXXXX";
	static int made;

	if (!made) {
		if (mkdtemp(home) == NULL)
			fatal("mkdtemp");
		made = 1;
	}

	return home;
}

/*
 * Run a script (may contain embedded newlines) through asccalc and return
 * its combined stdout/stderr. Exits the test if asccalc died on a signal.
 */
static char *
run_expr(const char *expr)
{
	char in_template[] = "/tmp/asccalc-in-XXXXXX";
	char out_template[] = "/tmp/asccalc-out-XXXXXX";
	char cmd[1024];
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

	if (snprintf(cmd, sizeof(cmd),
	    "HOME='%s' timeout 20 ./asccalc < '%s' > '%s' 2>&1",
	    scratch_home(), in_template, out_template) >= (int)sizeof(cmd)) {
		fprintf(stderr, "command buffer too small\n");
		exit(1);
	}

	status = system(cmd);
	if (status == -1)
		fatal("system");
	if (WIFSIGNALED(status)) {
		fprintf(stderr, "asccalc terminated by signal %d while running:\n%s\n",
		    WTERMSIG(status), expr);
		exit(1);
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) >= 124) {
		/* timeout(1) reports 124 for a timeout and 128+N for a signal */
		fprintf(stderr, "asccalc timed out or crashed (status %d) while running:\n%s\n",
		    WEXITSTATUS(status), expr);
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

/* The script must print a line containing "error" and must not crash. */
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

static void
expect_contains(const char *expr, const char *needle)
{
	char *output;

	output = run_expr(expr);
	if (strstr(output, needle) == NULL) {
		fprintf(stderr, "FAIL: %s\nexpected to contain: %s\nactual: %s\n",
		    expr, needle, output);
		failures++;
	}
	free(output);
}

static int
finish(const char *suite)
{
	if (failures != 0) {
		fprintf(stderr, "%s: %d test(s) failed\n", suite, failures);
		return 1;
	}

	printf("%s tests passed\n", suite);
	return 0;
}

#endif

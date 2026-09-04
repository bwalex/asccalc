#ifndef _PARSE_CTX_H
#define _PARSE_CTX_H

struct parse_ctx {
	const char *filename;
	void *scanner;
	void *buf;	/* YY_BUFFER_STATE of the line being scanned, if any */

	int interactive;

	int nesting;
	int linecont;
	int silent;
};
#endif

#ifndef _PARSE_CTX_H
#define _PARSE_CTX_H

struct parse_ctx {
	char *filename;
	void *scanner;

	int interactive;

	int nesting;
	int linecont;
	int silent;
};
#endif


#ifndef __UTIL_H_
#define __UTIL_H_

extern int u_quiet;

void u_debug(const char *fmt, ...);
void u_message(const char *fmt, ...);
void u_error(const char *fmt, ...);
void u_get_file_base(char *base, const char *path);
const char *u_get_file_ext(const char *path);

#define FRAND() (rand() / (double)RAND_MAX)
#define LERP(a, b, t) ((a) * (1 - (t)) + (b) * (t))

#define BILERP(a, b, c, d, s, t) ( \
	(a) * (1 - (s)) * (1 - (t)) \
	+ (b) * (s) * (1 - (t)) \
	+ (c) * (1 - (s)) * (t) \
	+ (d) * (s) * (t))

#endif


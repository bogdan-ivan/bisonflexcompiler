#ifndef __common_h
#define __common_h

#define NELEMS(x) (sizeof(x)/sizeof((x)[0]))
#define LOCAL static inline
#define dev_null(x) (void)(x)

#endif // !__common_h
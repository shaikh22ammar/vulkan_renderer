#ifndef ALIGN_H
#define ALIGN_H

#define GCD(a, b) ({					\
	typeof(a) _a = (a);				\
	typeof(b) _b = (b);				\
	while (_b) {					\
		typeof(_a) _t = _b;			\
		_b = _a % _b;				\
		_a = _t;				\
	}						\
	_a;						\
})

#define LCM(a, b) ({					\
	typeof(a) _a = (a);				\
	typeof(b) _b = (b);				\
	(_a / GCD(_a, _b)) * _b;			\
})

/* rounds x up to the nearest multiple of a,
 * where a is a power of 2 */
#define ALIGN_UP(x, a)  (((x) + (a) - 1) & ~((a) - 1))

#endif

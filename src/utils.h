#ifndef UTILS_H
#define UTILS_H

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

// LCM macro using GCD
#define LCM(a, b) ({					\
	typeof(a) _a = (a);				\
	typeof(b) _b = (b);				\
	(_a / GCD(_a, _b)) * _b;			\
})

#endif

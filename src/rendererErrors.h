#ifndef RENDERER_ERROR_H
#define RENDERER_ERROR_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// renderer return codes
typedef enum {
	RENDERER_SUCCESS =  0,
	RENDERER_ERR_VULKAN = -1,
	RENDERER_ERR_GLFW = -2,
	RENDERER_ERR_INVALID_ARG = -3,
	RENDERER_ERR_NOT_INITIALIZED = -4,
	RENDERER_ERR_OUT_OF_MEMORY = -5,
	RENDERER_ERR_SHADERS = -6,
	RENDERER_ERR_READ_FILE = -7,
	RENDERER_ERR_INCOMPATIBILITY= -8,
	RENDERER_ERR_IMAGE = -9,
	RENDERER_ERR_UNKNOWN = -80,
} RendererResult;

static inline const char* rendererResultString(RendererResult r) {
	switch(r) {
		case RENDERER_SUCCESS:
			return "RENDERER_SUCCESS";
			break;
		case RENDERER_ERR_VULKAN:
			return "RENDERER_ERR_VULKAN";
			break;
                case RENDERER_ERR_GLFW:
			return "RENDERER_ERR_GLFW";
			break;
		case RENDERER_ERR_INVALID_ARG:
			return "RENDERER_ERR_INVALID_ARG";
			break;
		case RENDERER_ERR_NOT_INITIALIZED:
			return "RENDERER_ERR_NOT_INITIALIZED";
			break;
		case RENDERER_ERR_OUT_OF_MEMORY:
			return "RENDERER_ERR_OUT_OF_MEMORY";
			break;
                case RENDERER_ERR_SHADERS:
			return "RENDERER_ERR_SHADERS";
                	break;
		case RENDERER_ERR_READ_FILE:
			return "RENDERER_ERR_READ_FILE";
			break;
		case RENDERER_ERR_INCOMPATIBILITY:
			return "RENDERER_ERR_INCOMPATIBILITY";
			break;
		case RENDERER_ERR_UNKNOWN:
			return "RENDERER_ERR_UNKNOWN";
			break;
                case RENDERER_ERR_IMAGE:
			return "RENDERER_ERR_IMAGE"; 
                  	break;
                }
        return "";
}

// error info
typedef struct RendererErrorInfo {
	RendererResult result; 
	int rawCode;	// raw vulkan error
	const char *function;	// __func__ name where error was detected
	const char *file;		// __FILE__ name where error was detected	
	int line;		// __LINE__ number where error was detected
	char message[256];	// some message
} RendererErrorInfo;


// returns pointer to the last error that was detected
const RendererErrorInfo *rendererGetLastError(void);

// configurable callback function executed on error
typedef void (*pFnRendererErrorCallback)(
	const RendererErrorInfo	*info,
	void			*userData
);
void rendererSetErrorCallback(pFnRendererErrorCallback cb, void *userData);

							/* everytime an error is detected: */
extern thread_local RendererErrorInfo gLastError; 	/* it is written to this variable, */
extern pFnRendererErrorCallback gErrorCb;		/* this function is called, */
extern void *gErrorCbUserdata;				/* with this as user data */

// sets an error to gLastError
static inline RendererResult setError(
	RendererResult	result,
	int rawCode,
	const char *func,
	const char *file,
	int line,
	const char *fmt, ...) {

	gLastError.result = result;
	gLastError.rawCode = rawCode;
	gLastError.function = func;
	gLastError.file = file;
	gLastError.line = line;
 
	if(*fmt != '\0'){
		va_list args;
		va_start(args, fmt);
		vsnprintf(gLastError.message, sizeof(gLastError.message), fmt, args);
		va_end(args);
	} else {
		snprintf(gLastError.message, sizeof(gLastError.message), "%s", rendererResultString(result));
	}
 
	if (gErrorCb)
		gErrorCb(&gLastError, gErrorCbUserdata);
 
	return result;
}
   
/* Set error with source location */
#define RR_SET_ERROR(result, raw, ...) \
	setError(result, raw, __func__, __FILE__, __LINE__, "" __VA_ARGS__)

/* The _CHECK macros will check for an error,
 * if and error is detected, it will write it and return the corresponding error code
 * The _WRITE macros will only write the error and evaluate to the error code instead of returning,
 * used when cleanup is required before returning, but it must not be used with expressions.
 * The _CHECK macros can optionally take two additional arguments ([resultVar], [cleanupLabel]): 
 * if an error is detected it will assign the error to variable [resultVar] to go to label [cleanupLabel]. */

#define RR_COUNT(_1, _2, _3, N, ...) N			// evaluates to the 4th argument
#define RR_NARGS(...) RR_COUNT(__VA_ARGS__, 3, 2, 1)	// counts if one argument has been passed or 3 (or 2)

#define VK_WRITE(vr)										\
	((vr) < 0 										\
		? setError(RENDERER_ERR_VULKAN, (int)(vr), __func__, __FILE__, __LINE__, "")	\
		: RENDERER_SUCCESS) 




#define VK_CHECK_1(expr)							\
	do {									\
		VkResult _vr = (expr);						\
		if (_vr < 0) {							\
			return setError(					\
				RENDERER_ERR_VULKAN, (int)_vr,			\
				__func__, __FILE__, __LINE__,			\
				"%s returned VkResult %d", #expr, (int)_vr);	\
		}								\
	} while (0)
#define VK_CHECK_3(expr, resultVar, cleanupLabel) 	\
	do { 						\
		VkResult _vr = (expr); 			\
		(resultVar) = VK_WRITE(_vr); 		\
		if ((resultVar) != RENDERER_SUCCESS) 	\
			goto cleanupLabel;		\
	} while (0)

#define VK_CHECK_DISPATCH(N, ...) VK_CHECK_##N(__VA_ARGS__)
#define VK_CHECK_EXPAND(N, ...) VK_CHECK_DISPATCH(N, __VA_ARGS__)
#define VK_CHECK(...) VK_CHECK_EXPAND(RR_NARGS(__VA_ARGS__), __VA_ARGS__)


#define RF_CHECK_1(expr)								\
	do {										\
		ReadFileResult _vr = (expr);						\
		if (_vr < 0) {								\
			return setError(						\
				RENDERER_ERR_READ_FILE, (int)_vr,			\
				__func__, __FILE__, __LINE__,				\
				"%s returned ReadFileResult %d", #expr, (int)_vr);	\
		}									\
	} while (0)

#define RF_CHECK_3(expr, resultVar, cleanupLabel)					\
	do {										\
		ReadFileResult _vr = (expr);						\
		if (_vr < 0) {								\
			(resultVar) = RENDERER_ERR_READ_FILE;				\
			setError(							\
				(resultVar), (int)_vr,					\
				__func__, __FILE__, __LINE__,				\
				"%s returned ReadFileResult %d", #expr, (int)_vr);	\
				goto cleanupLabel;					\
		}									\
	} while (0)
#define RF_CHECK_DISPATCH(N, ...) RF_CHECK_##N(__VA_ARGS__)
#define RF_CHECK_EXPAND(N, ...) RF_CHECK_DISPATCH(N, __VA_ARGS__)
#define RF_CHECK(...) RF_CHECK_EXPAND(RR_NARGS(__VA_ARGS__), __VA_ARGS__)


#define MALLOC_CHECK_1(p)							\
	do { 								\
		if (!(p)) {						\
			RR_SET_ERROR(RENDERER_ERR_OUT_OF_MEMORY, 0);	\
			return RENDERER_ERR_OUT_OF_MEMORY;		\
		}							\
	} while(0)
#define MALLOC_CHECK_3(p, resultVar, cleanupLabel)							\
	do { 								\
		if (!(p)) {						\
			(resultVar) = RENDERER_ERR_OUT_OF_MEMORY;		\
			RR_SET_ERROR((resultVar), 0);			\
			goto cleanupLabel;				\
		}							\
	} while(0)
#define MALLOC_CHECK_DISPATCH(N, ...) MALLOC_CHECK_##N(__VA_ARGS__)
#define MALLOC_CHECK_EXPAND(N, ...) MALLOC_CHECK_DISPATCH(N, __VA_ARGS__)
#define MALLOC_CHECK(...) MALLOC_CHECK_EXPAND(RR_NARGS(__VA_ARGS__), __VA_ARGS__)


/* Simply returns if an error is detected,
 * Used for propagating up a RendererResult from an internal call */
#define RR_TRY_1(expr)					\
	do {						\
		RendererResult _r = (expr);		\
		if (_r != RENDERER_SUCCESS)		\
			return _r;			\
	} while (0)
#define RR_TRY_3(expr, resultVar, cleanupLabel)		\
	do {						\
		(resultVar) = (expr);			\
		if (resultVar != RENDERER_SUCCESS)	\
			goto cleanupLabel;		\
	} while (0)
#define RR_TRY_DISPATCH(N, ...) RR_TRY_##N(__VA_ARGS__)
#define RR_TRY_EXPAND(N, ...) RR_TRY_DISPATCH(N, __VA_ARGS__)
#define RR_TRY(...) RR_TRY_EXPAND(RR_NARGS(__VA_ARGS__), __VA_ARGS__)


 
#ifdef __cplusplus
}
#endif

#endif

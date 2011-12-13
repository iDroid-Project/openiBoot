#ifndef  ERROR_H
#define  ERROR_H

#define ERROR_BIT			(0x80000000)
#define ERROR(x)			((x) | ERROR_BIT)
#define ERROR_CODE(x)		((x) &~ ERROR_BIT)
#define FAILED(x)			(((x) & ERROR_BIT) != 0)
#define SUCCEEDED(x)		(((x) & ERROR_BIT) == 0)
#define SUCCESS_VALUE(x)	((x) &~ ERROR_BIT)

#define FAIL_ON(x, y)			do { if(x) { return y; } } while(0)
#define SUCCEED_ON(x)			FAIL_ON((x), SUCCESS)
#define CHAIN_FAIL(x)			do { error_t __val = (x); if(FAILED(__val)) { return __val; } } while(0)
#define CHAIN_FAIL_STORE(x, y)	do { if(SUCCEEDED(x)) { x = (y); } } while(0)

enum
{
	SUCCESS = 0,
	EINVAL = ERROR(1),
	EIO = ERROR(2),
	ENOENT = ERROR(3),
	ENOMEM = ERROR(4),
	ETIMEDOUT = ERROR(5),
};

typedef uint32_t error_t;

#endif //ERROR_H

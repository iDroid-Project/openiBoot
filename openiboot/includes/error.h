#ifndef  ERROR_H
#define  ERROR_H

#define ERROR_BIT		(0x80000000)
#define ERROR(x)		((x) | ERROR_BIT)
#define ERROR_CODE(x)	((x) &~ ERROR_BIT)
#define FAILED(x)		(((x) & ERROR_BIT) != 0)
#define SUCCEEDED(x)	(((x) & ERROR_BIT) == 0)

enum
{
	SUCCESS = 0,
	EINVAL = ERROR(1),
	EIO = ERROR(2),
	ENOENT = ERROR(3),
	ENOMEM = ERROR(4),
};

typedef uint32_t error_t;

#endif //ERROR_H

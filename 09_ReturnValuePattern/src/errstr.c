#include "errstr.h"

#include <errno.h>

const char *errstr(int ret)
{
	if (ret >= 0) {
		return "OK";
	}

	/* switch trên macro, không trên con số: số của ETIMEDOUT trên Windows khác Linux */
	switch (-ret) {
	case EINVAL:
		return "EINVAL";
	case EIO:
		return "EIO";
	case ENXIO:
		return "ENXIO";
	case ENODEV:
		return "ENODEV";
	case EAGAIN:
		return "EAGAIN";
	case ETIMEDOUT:
		return "ETIMEDOUT";
	case ENOSPC:
		return "ENOSPC";
	default:
		return "E???";
	}
}

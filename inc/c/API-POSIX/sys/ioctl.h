#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#define MCCA_DEVCTL_GET_BLOCK_SIZE 1u
#define MCCA_DEVCTL_GET_UNIT_COUNT 2u
#define MCCA_DEVCTL_GET_BYTE_SIZE 3u

#ifdef __cplusplus
extern "C" {
#endif

int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif

#endif

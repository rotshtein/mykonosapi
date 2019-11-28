#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/*
 * fpga.c
 *
 *  Created on: Nov 18, 2019
 *      Author: x300
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define DEV             "/dev/mem"
#define HW_REGS_BASE    0xFF200000
#define HW_REGS_LEN     0x70000
#define HW_REGS_MASK    HW_REGS_LEN-1

static	int           _fd = 0;
static	void          *_virtual_base = NULL;

typedef enum t_FPGA_STATUS
{
	FPGA_OK,
	FPGA_FAILED
}FPGA_STATUS;

FPGA_STATUS FPGA_INIT();
//=============================================
FPGA_STATUS FPGA_INIT()
{
	_fd = open(DEV, O_RDWR | O_SYNC);
	if (_fd == -1)
	{
		printf("ERROR: could not open \"%s\" : %s\n", DEV, strerror(errno));
		return FPGA_FAILED;
	}

	// Map the address space for the Lightweight bridge into user space so we can interact with them.
	_virtual_base = mmap(NULL,                     /* addr */
		HW_REGS_LEN,              /* length */
		PROT_READ | PROT_WRITE,     /* prot */
		MAP_SHARED,               /* flags */
		_fd,                       /* fd */
		HW_REGS_BASE);            /* offset */

	if (_virtual_base == MAP_FAILED)
	{
		printf("ERROR: mmap() failed...\n");
		close(_fd);
		_fd = 0;
		_virtual_base = NULL;
		return FPGA_FAILED;
	}

	return FPGA_OK;
}

void FPGA_CLOSE()
{
	if (_fd != 0)
	{
		close(_fd);
		_fd = 0;
		_virtual_base = NULL;
	}
}

void IOWR_32DIRECT(uint32_t base, uint32_t reg_addr, uint32_t reg_val)
{
	if (_virtual_base == NULL)
	{
		if (FPGA_INIT() != FPGA_OK)
		{
			return;
		}
	}

	*(unsigned int*)(_virtual_base + base + reg_addr) = reg_val;
}

uint32_t IORD_32DIRECT(uint32_t base, uint32_t reg_addr)
{
	if (_virtual_base == NULL)
	{
		if (FPGA_INIT() != FPGA_OK)
		{
			return -1;
		}
	}
	
	return  *(unsigned int*)(_virtual_base + base + reg_addr);
}
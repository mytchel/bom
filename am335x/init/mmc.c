#include "../include/libc.h"

#define MMCHS0	0x48060000
#define MMCHS2	0x47810000
#define MMC1	0x481D8000

struct mmc {
	uint32_t pad[68];
	uint32_t sysconfig;
	uint32_t sysstatus;
	uint32_t pad1[3];
	uint32_t csre;
	uint32_t systest;
	uint32_t con;
	uint32_t pwcnt;
	uint32_t pad2[52];
	uint32_t sdmasa;
	uint32_t blk;
	uint32_t arg;
	uint32_t cmd;
	uint32_t rsp10;
	uint32_t rsp32;
	uint32_t rsp54;
	uint32_t rsp76;
	uint32_t data;
	uint32_t pstate;
	uint32_t hctl;
	uint32_t sysctl;
	uint32_t stat;
	uint32_t ie;
	uint32_t ise;
	uint32_t ac12;
	uint32_t capa;
	uint32_t pad3[1];
	uint32_t curcapa;
	uint32_t pad4[2];
	uint32_t fe;
	uint32_t admaes;
	uint32_t admasal;
	uint32_t admasah;
	uint32_t pad5[40];
	uint32_t rev;
};

static volatile struct mmc *mmc0, *mmc1, *mmc2;

int
mmc(void)
{
	size_t size = 4 * 1024;
	
	printf("Should init MMC\n");

	mmc0 = (struct mmc *) getmem(MEM_io, (void *) MMCHS0, &size);
	mmc1 = (struct mmc *) getmem(MEM_io, (void *) MMCHS2, &size);
	mmc2 = (struct mmc *) getmem(MEM_io, (void *) MMC1, &size);
	
	printf("mmc0 status = 0b%b\n", mmc0->sysstatus);
	printf("mmc1 status = 0b%b\n", mmc1->sysstatus);
	printf("mmc2 status = 0b%b\n", mmc2->sysstatus);
	
	return 0;
}

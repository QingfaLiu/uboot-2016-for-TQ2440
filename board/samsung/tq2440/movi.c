#include <config.h>
#include <common.h>
#include <asm/sections.h>

/* NAND FLASH控制器 */
#define BWSCON (*((volatile unsigned long *)0x48000000))
#define NFCONF (*((volatile unsigned long *)0x4E000000))
#define NFCONT (*((volatile unsigned long *)0x4E000004))
#define NFCMMD (*((volatile unsigned char *)0x4E000008))
#define NFADDR (*((volatile unsigned char *)0x4E00000C))
#define NFDATA (*((volatile unsigned char *)0x4E000010))
#define NFSTAT (*((volatile unsigned char *)0x4E000020))

static int isBootFromNorFlash(void)
{
	return BWSCON & (0x3 << 1);
}

void clear_bss(void)
{
	int *p = (int *)&__bss_start;
	while(p < (int *)&__bss_end) {
		*p++ = 0;
	}
}

void nand_init_ll(void)
{
#define TACLS   0
#define TWRPH0  2
#define TWRPH1  0
	/* 设置时序 */
	NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
	/* 使能NAND Flash控制器, 初始化ECC, 禁止片选 */
	NFCONT =(1<<4)|(1<<1)|(1<<0);
}

static void nand_select(void)
{
	NFCONT &= ~(1<<1);
}

static void nand_deselect(void)
{
	NFCONT |= (1<<1);    
}

static void nand_cmd(unsigned char cmd)
{
	volatile int i;
	NFCMMD = cmd;
	for (i = 0; i < 10; i++);
}

static void nand_addr(unsigned int addr)
{
	unsigned int col  = addr % 2048;
	unsigned int page = addr / 2048;
	volatile int i;

	NFADDR = col & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (col >> 8) & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = page & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (page >> 8) & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (page >> 16) & 0xff;
	for (i = 0; i < 10; i++); 
}

static void nand_wait_ready(void)
{
	while (!(NFSTAT & 1));
}

static unsigned char nand_data(void)
{
	return NFDATA;
}

void nand_read_ll(unsigned int addr, char *buf, size_t len)
{
	int col = addr % 2048;
	int i = 0;
	/* 1. 选中 */
	nand_select();
	while (i < len) {
		/* 2. 发出读命令00h */
		nand_cmd(0x00);
		/* 3. 发出地址(分5步发出) */
		nand_addr(addr);
		/* 4. 发出读命令30h */
		nand_cmd(0x30);
		/* 5. 判断状态 */
		nand_wait_ready();
		/* 6. 读数据 */
		for (; (col < 2048) && (i< len); col++) {
			buf[i] = nand_data();
			i++;
			addr++;
		}
		col = 0;
	}
	/* 7. 取消选中 */           
	nand_deselect();
}

void Copy2Sdram(char *src, char *dest, size_t len)
{
	/* 如果是NOR启动 */
	if (isBootFromNorFlash()) {
		while (len > 0) {
			*dest++ = *src++;
			--len;
		}
	} else {
		nand_init_ll();
		nand_read_ll((unsigned int)src, dest, len);
	}
}

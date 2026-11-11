#ifndef __DSP2837XSFLASH_H
#define __DSP2837XSFLASH_H

#include "F2837xS_device.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 芯片适配：参考工程（F28003x，每扇区 8K 字）的扇区表换成本芯片的真实扇区。
 * F28374S：A~D、K~N 为 8K 字节（0x2000 字）；E~J 为 32K 字节（0x8000 字）。 */
#define SectorA_start               0x80000U    /* 8K  */
#define SectorB_start               0x82000U    /* 8K  */
#define SectorC_start               0x84000U    /* 8K  */
#define SectorD_start               0x86000U    /* 8K  */
#define SectorE_start               0x88000U    /* 32K */
#define SectorF_start               0x90000U    /* 32K */
#define SectorG_start               0x98000U    /* 32K */
#define SectorH_start               0xA0000U    /* 32K */
#define SectorI_start               0xA8000U    /* 32K */
#define SectorJ_start               0xB0000U    /* 32K */
#define SectorK_start               0xB8000U    /* 8K  */
#define SectorL_start               0xBA000U    /* 8K  */
#define SectorM_start               0xBC000U    /* 8K  */
#define SectorN_start               0xBE000U    /* 8K  */

/* 本工程的分区起点（与 CMD/2837xS_FLASH_APP_CLA_lnk.cmd 一致） */
#define FlashBootStartAddress       0x80000U
#define FlashAppStartAddress        0x86000U
#define FlashBackupStartAddress     0xA0000U
#define FlashCfgStartAddress        0xBE000U

/* 备份区容量（16 位字）：H/I/J 各 0x8000 + K 0x2000 */
#define FlashBackupAreaWords        0x1A000U

/*Sector length in number of 16bits*/
#define SectorSmall_u16length       0x2000U    /* 8K 字节的扇区 */
#define SectorLarge_u16length       0x8000U    /* 32K 字节的扇区 */
#define Bank128KB_u16length         0x10000U

//Sector length in number of 32bits
#define Sector8KB_u32length         0x800U

//
// FMC Registers, Masks and Values
//
#define FLASH_O_ACC_EP           0x148U    // Flash Accumulated Erase Pulses
#define FLASH_O_ERA_PUL          0x136U    // Flash Max Erase Pulses

#define FLASH_ERA_PUL_MAX_ERA_PUL_M      0xFFFU  // Flash Max Erase Pulses Mask
#define MAX_ERASE_PULSE                  0x7D0U  // Flash Max Erase Pulses



#define DATA_BUFF_SIZE 1024
#define RETRY 5

/* F021 初始化需要系统时钟（MHz） */
#define FLASH_HCLK_MHZ   200U


extern int16 Dsp2837xSFlashInit(void);

/* RAM 内喂狗：擦写等待循环与 Fapi_serviceWatchdogTimer() 都调它，
 * 因为 Hw_FeedWatchDog() 链接在 Flash。 */
extern void FlashFeedWatchDog(void);

/* 等待超时且 FSM 仍忙时用看门狗复位整机（RAM 内执行，不返回 Flash 调用者） */
extern void FlashForceReset(void);

extern int16 EraseSingleSector(Uint32 u32SectorAddr);

extern int16 WriteFlashData(Uint32 u32TagAddr, void *pu16DataBuff, Uint32 u32DataLen);

extern int16 WriteFourWord(Uint32 u32TagAddr, Uint16 *pu16DataBuff);

extern int16 ReadFlashData(Uint32 u32TagAddr, Uint16 *pu16DataBuff, Uint32 u32DataLen);

#ifdef __cplusplus
}
#endif


#endif

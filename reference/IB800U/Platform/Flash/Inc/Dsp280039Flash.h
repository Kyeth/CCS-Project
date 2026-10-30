#ifndef __DSP280039FLASH_H
#define __DSP280039FLASH_H

#include "F28003x_device.h"


#ifdef __cplusplus
extern "C" {
#endif

// Bank0 Sector start addresses
#define FlashBank0StartAddress      0x80000U
#define Bzero_Sector0_start         0x80000U
#define Bzero_Sector1_start         0x81000U
#define Bzero_Sector2_start         0x82000U
#define Bzero_Sector3_start         0x83000U
#define Bzero_Sector4_start         0x84000U
#define Bzero_Sector5_start         0x85000U
#define Bzero_Sector6_start         0x86000U
#define Bzero_Sector7_start         0x87000U
#define Bzero_Sector8_start         0x88000U
#define Bzero_Sector9_start         0x89000U
#define Bzero_Sector10_start        0x8A000U
#define Bzero_Sector11_start        0x8B000U
#define Bzero_Sector12_start        0x8C000U
#define Bzero_Sector13_start        0x8D000U
#define Bzero_Sector14_start        0x8E000U
#define Bzero_Sector15_start        0x8F000U
#define FlashBank0EndAddress        0x8FFFFU

#define FlashBank1StartAddress     0x90000U
#define Bone_Sector0_start         0x90000U
#define Bone_Sector1_start         0x91000U
#define Bone_Sector2_start         0x92000U
#define Bone_Sector3_start         0x93000U
#define Bone_Sector4_start         0x94000U
#define Bone_Sector5_start         0x95000U
#define Bone_Sector6_start         0x96000U
#define Bone_Sector7_start         0x97000U
#define Bone_Sector8_start         0x98000U
#define Bone_Sector9_start         0x99000U
#define Bone_Sector10_start        0x9A000U
#define Bone_Sector11_start        0x9B000U
#define Bone_Sector12_start        0x9C000U
#define Bone_Sector13_start        0x9D000U
#define Bone_Sector14_start        0x9E000U
#define Bone_Sector15_start        0x9F000U
#define FlashBank1EndAddress       0x9FFFFU

#define FlashBank2StartAddress     0xA0000U
#define Btwo_Sector0_start         0xA0000U
#define Btwo_Sector1_start         0xA1000U
#define Btwo_Sector2_start         0xA2000U
#define Btwo_Sector3_start         0xA3000U
#define Btwo_Sector4_start         0xA4000U
#define Btwo_Sector5_start         0xA5000U
#define Btwo_Sector6_start         0xA6000U
#define Btwo_Sector7_start         0xA7000U
#define Btwo_Sector8_start         0xA8000U
#define Btwo_Sector9_start         0xA9000U
#define Btwo_Sector10_start        0xAA000U
#define Btwo_Sector11_start        0xAB000U
#define Btwo_Sector12_start        0xAC000U
#define Btwo_Sector13_start        0xAD000U
#define Btwo_Sector14_start        0xAE000U
#define Btwo_Sector15_start        0xAF000U
#define FlashBank2EndAddress       0xAFFFFU

//Sector length in number of 16bits
#define Sector8KB_u16length   0x1000U
#define Bank128KB_u16length   0x10000U

//Sector length in number of 32bits
#define Sector8KB_u32length   0x800U

//
// FMC Registers, Masks and Values
//
#define FLASH_O_ACC_EP           0x148U    // Flash Accumulated Erase Pulses
#define FLASH_O_ERA_PUL          0x136U    // Flash Max Erase Pulses

#define FLASH_ERA_PUL_MAX_ERA_PUL_M      0xFFFU  // Flash Max Erase Pulses Mask
#define MAX_ERASE_PULSE                  0x7D0U  // Flash Max Erase Pulses



#define DATA_BUFF_SIZE 1024
#define RETRY 5



extern void Dsp280039FlashInit(void);


extern int16 EraseSingleSector(Uint32 u32SectorAddr);

extern int16 WriteFlashData(Uint32 u32TagAddr, void *pu16DataBuff, Uint32 u32DataLen);
extern int16 WriteFourWord(Uint32 u32TagAddr, Uint16 *pu16DataBuff);


#ifdef __cplusplus
}
#endif


#endif

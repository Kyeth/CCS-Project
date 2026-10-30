#include "Sector.h"
#include "flash_programming_c28.h"
#include "Constants\Constants.h "
#include "Registers_C28x.h  "
#include "F021.h"
const Uint32 gSectZeroAddress[14] = { Bzero_Sector0_start,
                                 Bzero_Sector1_start,
                                 Bzero_Sector2_start,
                                 Bzero_Sector3_start,
                                 Bzero_Sector4_start,
                                 Bzero_Sector5_start,
                                 Bzero_Sector6_start,
                                 Bzero_Sector7_start,
                                 Bzero_Sector8_start,
                                 Bzero_Sector9_start,
                                 Bzero_Sector10_start,
                                 Bzero_Sector11_start,
                                 Bzero_Sector12_start,
                                 Bzero_Sector13_start };

const Uint32 gSectOneAddress[14] = { Bone_SectorA_start,
                                 Bone_SectorB_start,
                                 Bone_SectorC_start,
                                 Bone_SectorD_start,
                                 Bone_SectorE_start,
                                 Bone_SectorF_start,
                                 Bone_SectorG_start,
                                 Bone_SectorH_start,
                                 Bone_SectorI_start,
                                 Bone_SectorJ_start,
                                 Bone_SectorK_start,
                                 Bone_SectorL_start,
                                 Bone_SectorM_start,
                                 Bone_SectorN_start };

const Uint16 gSectSize[14] = {  Bzero_16KSector_u32length,
                                Bzero_16KSector_u32length,
                                Bzero_16KSector_u32length,
                                Bzero_16KSector_u32length,
                                Bzero_64KSector_u32length,
                                Bzero_64KSector_u32length,
                                Bzero_64KSector_u32length,
                                Bzero_64KSector_u32length,
                                Bzero_64KSector_u32length,
                                Bzero_64KSector_u32length,
                                Bzero_16KSector_u32length,
                                Bzero_16KSector_u32length,
                                Bzero_16KSector_u32length,
                                Bzero_16KSector_u32length };


static Fapi_StatusType eraseFlashSector(uint32 *pu32StartAddress, uint32 len);

/*------------------------------------------------------------------
  Simple memory copy routine to move code out of flash into SARAM
-----------------------------------------------------------------*/

void Example_MemCopy(Uint16 *SourceAddr, Uint16* SourceEndAddr, Uint16* DestAddr)
{
    while(SourceAddr < SourceEndAddr)
    {
       *DestAddr++ = *SourceAddr++;
    }
    return;
}
/*------------------------------------------------------------------
  Init Flash Sectors
-----------------------------------------------------------------*/
void initFlashSectors(Uint16 bankx)
{
    Fapi_StatusType oReturnCheck;

    EALLOW;
    Flash0EccRegs.ECC_ENABLE.bit.ENABLE = 0x0;

    oReturnCheck = Fapi_initializeAPI(F021_CPU0_BASE_ADDRESS, 200);
    if(oReturnCheck != Fapi_Status_Success){
        asm(" NOP");
    }
    if(bankx == 0)
    {
//        oReturnCheck = Fapi_setActiveFlashBank(Fapi_FlashBank0);
//        if(oReturnCheck != Fapi_Status_Success){
//            asm(" NOP");
//        }
    }
    else if(bankx == 1)
    {
        oReturnCheck = Fapi_setActiveFlashBank(Fapi_FlashBank1);
        if(oReturnCheck != Fapi_Status_Success)
        {
            asm(" NOP");
        }
    }

    Flash0EccRegs.ECC_ENABLE.bit.ENABLE = 0xA;
    EDIS;
}
/******************************************************************************
Function:    sharedErase
Description: 擦除指定bank上的指定sector，
Input:       @bankx-bank编号,可以是0或者1;
             @sectors-要擦除的扇区号，每个bit代表一个扇区
Output:      无
return:      0-擦除成功;其他值擦除失败
******************************************************************************/
Uint16 sharedErase(Uint32 sectors, Uint16 bankx)
{
    int i = 0;
    int ret = 0;
    Fapi_StatusType oReturnCheck;

    if(0 != bankx && 1 != bankx)
    {
        return 1;
    }

    for(i = 0; i < 14; i++)
    {
        if( (sectors & 0x00000001) == 0x00000001 )
        {
            DINT;
            if(0 == bankx)
                oReturnCheck = eraseFlashSector((Uint32 *)(gSectZeroAddress[i]), gSectSize[i]);
            else if(1 == bankx)
                oReturnCheck = eraseFlashSector((uint32 *)(gSectOneAddress[i]), gSectSize[i]);
            EINT;

            if(oReturnCheck != Fapi_Status_Success)
            {
                ret++;
            }
        }
        sectors = sectors >> 1;
    }

    return ret;
}
/******************************************************************************
Function:    programFlashOneWord
Description: 对指定地址 destAddr 从 buffer 中写入 1 个 16bits 长度数据,
                                  这种方式下只能使用 Fapi_DataOnly模式，需要先将ECC禁止
Input:       @destAddr-指定要写入的flash地址;@buffer-写入内容的buffer;
Output:      无
return:      Fapi_Status_Success(枚举值为0)-写入成功;其他值表示写入失败
******************************************************************************/
#pragma CODE_SECTION(programFlashOneWord,".TI.ramfunc");
Fapi_StatusType programFlashOneWord(Uint32 destAddr, Uint16 *buffer)
{
    uint16 timeOutCnt = 0;
    volatile Fapi_FlashStatusType oFlashStatus;
    Fapi_StatusType oReturnCheck = Fapi_Status_Success;
    // Programming Flash
    EALLOW;

    oReturnCheck = Fapi_issueProgrammingCommand((Uint32 *)destAddr,
                   buffer,
                   4,
                   0,
                   0,
                   Fapi_AutoEccGeneration);

    while((Fapi_checkFsmForReady() == Fapi_Status_FsmBusy) &&
           (++timeOutCnt < 50000));

    if((oReturnCheck != Fapi_Status_Success) ||
            (timeOutCnt >= 50000))
    {
        EDIS;
        return Fapi_Status_FsmBusy;
    }

    oFlashStatus = Fapi_getFsmStatus();

    if(0 != oFlashStatus)
    {
        EDIS;
        return Fapi_Status_FsmBusy;
    }

    EDIS;
    return Fapi_Status_Success;
}
/******************************************************************************
Function:    programFlash
Description: 对指定地址 destAddr 从 buffer 中写入 len 个 16bits 长度数据
Input:       @destAddr-指定要写入的 flash 地址;@buffer-写入内容的buffer;
             @len-写入长度类型 16bits
Output:      无
return:      Fapi_Status_Success(枚举值为0)-写入成功;其他值表示写入失败
******************************************************************************/
#pragma CODE_SECTION(programFlash, ".TI.ramfunc");
Fapi_StatusType programFlash(Uint32 destAddr, Uint16 *buffer, Uint16 len)
{
    Uint32 i = 0;
    Uint16 timeOutCnt = 0;
    Uint32 u32Index = 0;
    Uint32 *pbuffer32 = (Uint32 *)buffer;
    volatile Fapi_FlashStatusType oFlashStatus;
    Fapi_FlashStatusWordType oFlashStatusWord;
    Fapi_StatusType oReturnCheck = Fapi_Status_Success;
    // Programming Flash
    EALLOW;
    Flash0EccRegs.ECC_ENABLE.bit.ENABLE = 0xA;// Enable ECC

    for(i = 0, u32Index = destAddr;
       (u32Index < (destAddr + len))
       && (oReturnCheck == Fapi_Status_Success); i+= 8, u32Index+= 8)
    {
        oReturnCheck = Fapi_issueProgrammingCommand((uint32 *)u32Index,
                       buffer + i,
                       8,
                       0,
                       0,
                       Fapi_AutoEccGeneration);

        while((Fapi_checkFsmForReady() == Fapi_Status_FsmBusy) &&
                (++timeOutCnt < 50000));

        if((oReturnCheck != Fapi_Status_Success) || (timeOutCnt >= 50000))
        {
            EDIS;
            return Fapi_Status_FsmBusy; // 执行失败
        }

        oFlashStatus = Fapi_getFsmStatus();

        if(0 != oFlashStatus)
        {
            EDIS;
            return Fapi_Status_FsmBusy; // 执行失败
        }

#if 1
        oReturnCheck = Fapi_doVerify((uint32 *)u32Index,
                       4,
                       pbuffer32 + (i/2),
                       &oFlashStatusWord);

        if(oReturnCheck != Fapi_Status_Success)
        {
            EDIS;
            return Fapi_Status_FsmBusy; // 执行失败
        }
#endif
    }

    EDIS;
    return Fapi_Status_Success; // 执行成功
}
/******************************************************************************
Function:    eraseFlashSector
Description: 对指定sector进行擦除
Input:       @pu32StartAddress-指定要擦除的sector地址;扇区sector的长度(字节长度8bits)
Output:      无
return:      Fapi_Status_Success(枚举值为0)-写入成功;其他值表示擦除失败
******************************************************************************/
#pragma CODE_SECTION(eraseFlashSector, ".TI.ramfunc");
static Fapi_StatusType eraseFlashSector(Uint32 *pu32StartAddress, Uint32 len)
{
    Uint16 timeOutCnt = 0;
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusWordType oFlashStatusWord;
    Fapi_FlashStatusType  oFlashStatus;

    EALLOW;
    // Erase Sector
    oReturnCheck = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector,
                   (Uint32 *)pu32StartAddress);

    // Wait until FSM is done with erase sector operation
    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady
            && timeOutCnt < 50000);

    if((oReturnCheck != Fapi_Status_Success) || (timeOutCnt >= 50000))
    {
        EDIS;
        return Fapi_Status_FsmBusy; // 执行失败
    }

    oFlashStatus = Fapi_getFsmStatus();
    if(0 != oFlashStatus)
    {
        EDIS;
        return Fapi_Status_FsmBusy; // 执行失败
    }

    // Verify that SectorL is erased.  The Erase step itself does a
    // verify as it goes.  This verify is a 2nd verification that can be done.
    oReturnCheck = Fapi_doBlankCheck((uint32 *)pu32StartAddress,
                   len,
                   &oFlashStatusWord);
    if(Fapi_Status_Success != oReturnCheck)
    {
        EDIS;
        return Fapi_Status_FsmBusy; // 执行失败
    }

    EDIS;
    return Fapi_Status_Success; // 执行成功
}


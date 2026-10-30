/*
 * Sector.h
 *
 *  Created on: 2022.5.3
 *      Author:
 */

#ifndef SECTOR_H_
#define SECTOR_H_


#include "F2837xS_device.h"
#include "F2837xS_Examples.h"
#include "flash_programming_c28.h"
#include "F021_F2837xD_C28x.h"
//#include "math.h"
//#include "stdlib.h"
//#include "bootFlash.h"
#include "Types.h"
#define SCIA_Sel

#define BootFLAG_BOOTLOADER   0x55BB
#define BootFLAG_COPY_BKTOAPP 0x00DD
#define BootFLAG_JUMPAPP      0xFFFF//0x55AA//0xFFFF//0x0

#define BootFLAG_BK_OK      0x55AA  //0x5303
#define BootFLAG_APP_OK     0x55AA  //0x5101


// APP 40K
#define APP_BANKX                0                      // bank0
#define APP_StartAddr            Bzero_Sector9_start
#define APP_EndAddr              Bzero_Sector10_End
#define APP_SECTORS             (BIT_SECTOR9 | BIT_SECTOR10)
#define APP_FLAG_SECTOR          BIT_SECTOR10             // APP闂傚倷绀侀幉锟犲礉閺嶎厽鍋￠柨鏇炲�归崑澶愭煥濠靛棛澧㈤柣銈忔嫹婵犲痉鏉匡拷鎾剁矆娓氾拷閵嗗倹绻濆顓犲幈闂佸搫绉撮悘姘憋拷姘炬嫹
#define APP_FLAG_EndAddr         Bzero_Sector10_End       // APP闂傚倷绀侀幉锟犲礉閺嶎厽鍋￠柨鏇炲�归崑澶愭煥濠靛棛澧㈤柣銈忔嫹婵犲痉鏉匡拷鎾绘倿閿曪拷鏁堥柣鎰劋閸婂爼鐓崶銊︾叆闁告柣鍊栫换婵嬫晸娴犲绠绘い鏂裤偢閺佹粓姊虹捄銊ユ珢闁瑰嚖鎷�

// Backup 40K
#define APP_BK_BANKX             0                      // bank0
#define APP_BK_StartAddr         Bzero_Sector3_start
#define APP_BK_EndAddr           Bzero_Sector4_End
#define APP_BK_SECTORS          (BIT_SECTOR3 | BIT_SECTOR4)
#define BK_FLAG_SECTOR           BIT_SECTOR4           // BK闂傚倷绀侀幉锟犲礉閺嶎厽鍋￠柨鏇炲�归崑澶愭煥濠靛棛澧㈤柣銈忔嫹婵犲痉鏉匡拷鎾剁矆娓氾拷閵嗗倹绻濆顓犲幈闂佸搫绋侀崢鍓ф娴煎瓨鐓欓柣褍鎽滅粙濠氭煙娓氬灝濡块柨鐔告灮缂嶅棙绂嶅┑瀣濡わ絽鍟悡鏇犳喐濠靛绠柨鐕傛嫹
#define BK_FLAG_EndAddr          Bzero_Sector4_End     // BK闂傚倷绀侀幉锟犲礉閺嶎厽鍋￠柨鏇炲�归崑澶愭煥濠靛棛澧㈤柣銈忔嫹婵犲痉鏉匡拷鎾绘倿閿曪拷鏁堥柣鎰劋閸婂爼鐓崶銊︾叆闁告柣鍊栫换婵嬫晸娴犲绠绘い鏂裤偢閺佹粓姊虹捄銊ユ珢闁瑰嚖鎷�

#define BootFlagStartFlagAddr  (BK_FLAG_EndAddr - 1)



#define BIT_SECTOR0                 (1L<<0) // A
#define BIT_SECTOR1                 (1L<<1) // B
#define BIT_SECTOR2                 (1L<<2) // C
#define BIT_SECTOR3                 (1L<<3) // D
#define BIT_SECTOR4                 (1L<<4) // E
#define BIT_SECTOR5                 (1L<<5) // F
#define BIT_SECTOR6                 (1L<<6) // G
#define BIT_SECTOR7                 (1L<<7) // H
#define BIT_SECTOR8                 (1L<<8) // I
#define BIT_SECTOR9                 (1L<<9) // J
#define BIT_SECTOR10                (1L<<10) // K
#define BIT_SECTOR11                (1L<<11) // L
#define BIT_SECTOR12                (1L<<12) // M
#define BIT_SECTOR13                (1L<<13) // N

extern int16  gUpdateFlag ;
extern Uint16 gWriteBuffer[4];
extern Uint16 readTemp  ;
extern  Uint16 timeOutCnt  ;
extern int32 DelayJumpCnt;
extern Uint16 KillDog;
extern Fapi_StatusType oReturnCheck;
void Example_MemCopy(Uint16 *SourceAddr, Uint16* SourceEndAddr, Uint16* DestAddr);
Fapi_StatusType programFlash(Uint32 destAddr, Uint16 *buffer, Uint16 len);
Uint16 sharedErase(Uint32 sectors, Uint16 bankx);
void initFlashSectors(Uint16 bankx);
Fapi_StatusType programFlashOneWord(Uint32 destAddr, Uint16 *buffer);
#endif /* MAIN_H_ */

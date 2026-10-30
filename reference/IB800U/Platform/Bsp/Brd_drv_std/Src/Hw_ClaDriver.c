#include "Hw_ClaDriver.h"
#include "memcfg.h"


#ifdef __cplusplus
    extern "C" {
#endif

#if defined(__TI_EABI__)

extern Uint32 Cla1ProgRunStart;
extern Uint32 Cla1ProgLoadStart;
extern Uint32 Cla1ProgLoadSize;

extern Uint32 Cla1ConstRunStart;
extern Uint32 Cla1ConstLoadStart;
extern Uint32 Cla1ConstLoadSize;

#else


extern Uint32 _Cla1ProgRunStart;
extern Uint32 _Cla1ProgLoadStart;
extern Uint32 _Cla1ProgLoadSize;


extern Uint32 _Cla1ConstRunStart;
extern Uint32 _Cla1ConstLoadStart;
extern Uint32 _Cla1ConstLoadSize;

#endif

void Hw_SingleClaInit(ST_CLA_CFG_ITEM *pstClaCfgItem)
{
    EALLOW;
    CLA_mapTaskVector(CLA1_BASE, (CLA_MVECTNumber)pstClaCfgItem->u16IntVectorNum, (uint16_t)pstClaCfgItem->ClaTask);
    CLA_setTriggerSource((CLA_TaskNumber)(pstClaCfgItem->u16TaskNo - 1), (CLA_Trigger)pstClaCfgItem->u16TrigerSrc);

    // Disable background task
    CLA_disableBackgroundTask(CLA1_BASE);
    CLA_enableTasks(CLA1_BASE, (1 << (pstClaCfgItem->u16TaskNo-1)));
    EDIS;
}

void Hw_ClaInit(ST_CLA_CFG *pstClaCfg)
{
    Uint16 i = 0;

    for (i = 0; i < pstClaCfg->u16ItemNum; i++)
    {
        Hw_SingleClaInit(&pstClaCfg->pstClaCfgItem[i]);
    }

    // 使能软件触发
    CLA_enableIACK(CLA1_BASE);

    EALLOW;

#if defined(__TI_EABI__)

    // 将CLA代码从FLASH加载到RAM中
    memcpy((Uint32 *)&Cla1ProgRunStart, (Uint32 *)&Cla1ProgLoadStart, (Uint32)&Cla1ProgLoadSize);
    memcpy((Uint32 *)&Cla1ConstRunStart, (Uint32 *)&Cla1ConstLoadStart, (Uint32)&Cla1ConstLoadSize);

#else
    // 将CLA代码从FLASH加载到RAM中
    memcpy((Uint32 *)&_Cla1ProgRunStart, (Uint32 *)&_Cla1ProgLoadStart, (Uint32)&_Cla1ProgLoadSize);
    memcpy((Uint32 *)&_Cla1ConstRunStart, (Uint32 *)&_Cla1ConstLoadStart, (Uint32)&_Cla1ConstLoadSize);

#endif


    // 初始化CLA to CPU的msgRAM
    MemCfgRegs.MSGxINIT.bit.INIT_CLA1TOCPU = 1;
    while(MemCfgRegs.MSGxINITDONE.bit.INITDONE_CLA1TOCPU != 1){};

    // 初始化CPU to CLA的msgRAM
    MemCfgRegs.MSGxINIT.bit.INIT_CPUTOCLA1 = 1;
    while(MemCfgRegs.MSGxINITDONE.bit.INITDONE_CPUTOCLA1 != 1){};

    EDIS;

    // 开放内存权限给CLA，默认内存权限只有CPU专用
    MemCfg_setLSRAMControllerSel(MEMCFG_SECT_LS4, MEMCFG_LSRAMCONTROLLER_CPU_CLA1);
    MemCfg_setLSRAMControllerSel(MEMCFG_SECT_LS5, MEMCFG_LSRAMCONTROLLER_CPU_CLA1);
    MemCfg_setLSRAMControllerSel(MEMCFG_SECT_LS6, MEMCFG_LSRAMCONTROLLER_CPU_CLA1);
    MemCfg_setLSRAMControllerSel(MEMCFG_SECT_LS7, MEMCFG_LSRAMCONTROLLER_CPU_CLA1);

    // 配置内存属性
    MemCfg_setCLAMemType(MEMCFG_SECT_LS4, MEMCFG_CLA_MEM_PROGRAM);
    MemCfg_setCLAMemType(MEMCFG_SECT_LS5, MEMCFG_CLA_MEM_PROGRAM);
    MemCfg_setCLAMemType(MEMCFG_SECT_LS6, MEMCFG_CLA_MEM_PROGRAM);
    MemCfg_setCLAMemType(MEMCFG_SECT_LS7, MEMCFG_CLA_MEM_DATA);
}

#ifdef __cplusplus
    extern "C" {
#endif

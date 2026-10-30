#include "Hw_ClockDriver.h"
#include "F2837xS_Examples.h"

Uint16 Hw_ClockInit(const ST_CLOCK_CFG *cfg)
{
    if (cfg == 0) { return 0U; }

    /* TI device support performs Flash RAM copy and calibration at boot. */
    InitSysCtrl();
    /* The Broad table selects the final PLL configuration. With this board's
     * 10 MHz/200 MHz settings the second call exits immediately in TI code. */
    InitSysPll(cfg->u16OscSource, cfg->u16PllImult,
               cfg->u16PllFmult, cfg->u16SysclkDiv);

    EALLOW;
    ClkCfgRegs.PERCLKDIVSEL.bit.EPWMCLKDIV = cfg->u16EpwmclkDiv;
    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = cfg->u16LspclkDiv;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0U;
    EDIS;

    if (ClkCfgRegs.CLKSRCCTL1.bit.OSCCLKSRCSEL != cfg->u16OscSource ||
        ClkCfgRegs.SYSPLLMULT.bit.IMULT != cfg->u16PllImult ||
        ClkCfgRegs.SYSPLLMULT.bit.FMULT != cfg->u16PllFmult ||
        ClkCfgRegs.SYSCLKDIVSEL.bit.PLLSYSCLKDIV != cfg->u16SysclkDiv ||
        ClkCfgRegs.PERCLKDIVSEL.bit.EPWMCLKDIV != cfg->u16EpwmclkDiv ||
        ClkCfgRegs.LOSPCP.bit.LSPCLKDIV != cfg->u16LspclkDiv)
    {
        return 0U;
    }
    return 1U;
}

void Hw_WatchdogEnable(void)
{
    ServiceDog();
    EALLOW;
    WdRegs.WDCR.all = 0x0028U | (WdRegs.WDCR.all & 0x0007U);
    EDIS;
}

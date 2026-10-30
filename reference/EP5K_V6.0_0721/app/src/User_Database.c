/*
 * User_Database.c
 *
 * */
//###########################################################################
#include "User_Database.h"
#include "Sample.h"
#include "Protect.h"
#include "User_Eeprom.h"
#include "InvStateMachine.h"
#include "LLCStateMachine.h"
#include "PVStateMachine.h"
#include "ConfigSystemPara.h"
//***************************************************************************
Uint16 Eeprom_shadow[Database_Num];
Uint16 Eeprom_Wshadow[(Database_Num+1)];// 写缓存,末尾作空闲标志

//Uint16 Eeprom_Wshadow[3][Database_Num];            // 写缓存

struct Database_REGS Database[Database_Num]={
//==================================================================================================
//地址          类型               读/写         缩放                  最大值                最小值                          指针地址
//==================================================================================================
{0,     'U',    'R',    1,       65535,     0,          &ErrArray_Num },
{1,     'U',    'R',    1,       65535,     0,          &SystemError.Fault1.all },
{2,     'U',    'R',    1,       65535,     0,          &SystemError.Fault2.all },
{3,     'U',    'R',    1,       65535,     0,          &SystemError.Fault3.all },
{4,     'U',    'R',    1,       65535,     0,          &SystemAlarm.all },
{5,     'U',    'R',    1,       65535,     0,          &PVSystemData.PV1Available },
{6,     'U',    'R',    1,       65535,     0,          &PVSystemData.PV2Available },
{7,     'U',    'R',    1,       65535,     0,          &PVSystemData.PV3Available },
{8,     'U',    'R',    1,       65535,     0,          &InvError.Inv1Err.all },
{9,     'U',    'R',    1,       65535,     0,          &InvError.Inv2Err.all },
{10,    'U',    'R',    1,       65535,     0,          &GridError.Grid1Err.all },
{11,    'U',    'R',    1,       65535,     0,          &GridError.Grid2Err.all },
{12,    'F',    'R',    1,       32767,     -32768,     &SampleIns.Bus1VoltIns },
{13,    'F',    'R',    1,       32767,     -32768,     &SampleIns.Bus2VoltIns },
{14,    'F',    'R',    1,       32767,     -32768,     &SampleIns.Inv1VoltIns },
{15,    'F',    'R',    10,       32767,     -32768,    &SampleIns.Inv1CurrIns },
{16,    'F',    'R',    1,       32767,     -32768,     &SampleIns.Inv2VoltIns },
{17,    'F',    'R',    10,       32767,     -32768,    &SampleIns.Inv2CurrIns },
{18,    'F',    'R',    1,       32767,     -32768,     &SampleIns.Grid1VoltIns },
{19,    'F',    'R',    1,       32767,     -32768,     &SampleIns.Grid2VoltIns },
{20,    'F',    'R',    1,       32767,     -32768,     &SampleIns.PV1VoltIns },
{21,    'F',    'R',    10,       32767,     -32768,    &SampleIns.PV1CurrIns },
{22,    'F',    'R',    1,       32767,     -32768,     &SampleIns.PV2VoltIns },
{23,    'F',    'R',    10,       32767,     -32768,    &SampleIns.PV2CurrIns },
{24,    'F',    'R',    1,       32767,     -32768,     &SampleIns.PV3VoltIns },
{25,    'F',    'R',    10,       32767,     -32768,    &SampleIns.PV3CurrIns },
{26,    'F',    'R',    1,       32767,     -32768,     &SampleIns.BUSVoltIns },
{27,    'F',    'R',    10,       32767,     -32768,    &SampleIns.Load1CurrIns },
{28,    'F',    'R',    10,       32767,     -32768,    &SampleIns.Load2CurrIns },
{29,    'U',    'R',    1,       65535,     0,          &InvStateData.WorkState },      // INV 状态机
{30,    'U',    'R',    1,       65535,     0,          &PV1WorkState.State },          // LLC 状态机
{31,    'U',    'R',    1,       65535,     0,          &PV1WorkState.State },          // PV1 状态机
{32,    'U',    'R',    1,       65535,     0,          &PV2WorkState.State },
{33,    'U',    'R',    1,       65535,     0,          &SystemPara.SysTimerCount },    // 系统定时器
{34,    'U',    'W',    1,       65535,     0,          &ErrArray_NO }
};

//***************************************************************************
Uint16 Database_Read(Uint16 i)  //读参数
{
    Uint16 Data;

    if (i >= Database_Num) //序号大于总数
    {
        return 0;
    }

    //否则读取Database表中的数据（可写的参数均可读）
    if ((Database[i].RW == 'R') || (Database[i].RW == 'W'))
    {
        if(Database[i].type == 'F')
        {
            Data = (int)(Database[i].scale*(*(float*)Database[i].p));
        }
        else if(Database[i].type == 'U')
        {
            Data=Database[i].scale*(*(Uint16*)Database[i].p);
        }
        else if(Database[i].type == 'I')
        {
            Data = Database[i].scale*(*(int*)Database[i].p);
        }

        return Data;
    }
    else
        return 0;
}
//***************************************************************************
void Database_Write(Uint16 i,Uint16 SetData) //写参数
{
    float temp;

    if (i >= Database_Num)
    {
        return;
    }

    if (Database[i].RW=='W')
    {
        if (Database[i].scale == 1)
        {
            temp = (int)SetData;
        }
        else
        {
            temp = (float)((int)SetData)/Database[i].scale;
        }

        if (temp < Database[i].low)
        {
            temp = Database[i].low;
        }
        else if(temp > Database[i].upper)
        {
            temp = Database[i].upper;
        }

//--------------------------------------------------------------------------
        if(Database[i].type == 'F')
        {
            *(float*)Database[i].p = (float)temp;
        }
        else if(Database[i].type=='I')
        {
            *(int*)Database[i].p = (int)temp;
        }
        else//if(Database[i].type=='U')
        {
            *(int*)Database[i].p = (Uint16)temp;
        }
    }
}
//***************************************************************************
void Database_Save(void)
{
    Uint16 i = 0;

    if(Eeprom_Wshadow[Database_Num] == 0)
    {
        for(i=0;i< Database_Num;i++)
        {
            Eeprom_Wshadow[i] = Database_Read(i);
        }

        Eeprom_Wshadow[Database_Num] = 1;
    }
}

//***************************************************************************
// 末尾作空闲标志
void Database_Clear(void)
{
    Eeprom_Wshadow[Database_Num] = 0;
}


//===========================================================================
// No more.
//===========================================================================

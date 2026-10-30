/*
 * User_Database.h
 *
 *  Created on: 2022年6月18日
 *      Author: tian
 */

#ifndef _USER_DATABASE_H_
#define _USER_DATABASE_H_

#include "F28x_Project.h"

#define Database_Num            35
//=============================================================================
struct Database_REGS{
//地址        变量类型    读/写     缩放因子        最大值         最小值     指针地址
Uint16 addr;    char type;  char RW;    Uint32 scale;   Uint16 upper;   int low;    void *p;
};

extern Uint16 Eeprom_shadow[Database_Num];
extern Uint16 Eeprom_Wshadow[(Database_Num+1)];
extern struct Database_REGS Database[Database_Num];

extern Uint16 Database_Read(Uint16 i);
extern void   Database_Write(Uint16 i,Uint16 SetData);

extern void   Database_Save(void);
extern void   Database_Clear(void);


#endif /* APP_INC_USER_DATABASE_H_ */

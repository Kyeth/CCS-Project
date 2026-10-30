/*
 * User_Eeprom.h
 *
 *  Created on: 2022年6月20日
 *      Author: tzz
 */

#ifndef _USER_EEPROM_H_
#define _USER_EEPROM_H_

extern Uint16 ErrArray_Num;                       //Eeprom中存放的故障数据的总数（+1）
extern Uint16 ErrArray_NO;                        //Eeprom中故障数组的序号

void User_Eeprom_Init(void);
void User_Eeprom(void);

#endif /* APP_INC_USER_EEPROM_H_ */

/*
 * Update.h
 *
 *  Created on: 2022Äê5ÔÂ5ÈÕ
 *      Author: tian
 */

#ifndef _UPDATE_H_
#define _UPDATE_H_

#include "stdint.h"

typedef struct
{
    uint16_t UpdateFlag;
    uint16_t JumpCount;
    uint16_t KillDog;
    uint16_t InitFlag;
    uint16_t WriteBuffer[4];
}UPDATE_DATA;

extern UPDATE_DATA UpdateData;

void CheckUpdateFlag(void);

#endif /* APP_INC_UPDATE_H_ */

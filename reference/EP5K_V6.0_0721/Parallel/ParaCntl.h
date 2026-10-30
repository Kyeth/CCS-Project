/*
 * ParaCntl.h
 *
 *  Created on: 2024骞�1鏈�24鏃�
 *      Author: DLadmin
 *      鐗堟湰锛歏2.0 PR021
 */

#ifndef MODULE_INVERTERCONTROL_H_
#define MODULE_INVERTERCONTROL_H_

void ParaControl(void);
void InitParaCntl(void);
void Init_DroopCoeffs(Uint16);
void OffGridPowerCal(void);
void Para_ResetWorkState(void);
#endif /* MODULE_INVERTERCONTROL_H_ */

/*
 * ParaProtocol.h
 *
 *  Created on: 2022妤犵儑鎷�9闁哄牞鎷�3闁哄喛鎷�
 *      Author: DLadmin
 */

#ifndef MODULE_PARAPROTOCOL_H_
#define MODULE_PARAPROTOCOL_H_

//Command List for CAN Communication
#define CmdWithId                0x01
#define CmdPwmSync               0x02
#define CmdPhaseSync             0x03
#define CmdSendP                 0x04   //CmdRlySync
#define CmdSendWorkState         0x05
#define CmdSendQ                 0x06
#define CmdSendGragh             0x07
#define CmdSendAging             0x08
#define ReplyWorkState           0x09

#define Cmd3PhasePowerP          0x14
#define Cmd3PhasePowerQ          0x15
#define CmdVoltAvgAdj            0x16
#define CmdVoltDcAdj             0x17
#define CmdDcCurrent             0x18
#define CmdPAvgAdj               0x19
//#define CmdBusVolt               0x1A
#define CmdDcPower               0x1B
#define CmdBatInfo               0x1C
#define CmdQAvgAdj               0x1E
#define CmdDcAvgAdj              0x1F

#define CmdSystemTurnOff         0x20

//#define CmdDeltaTheta            0x21
//#define CmdDeltaVolt             0x22
#define CmdMasterAssignLocation  0x24
#define CmdMasterInfo            0x25
#define InqNewModule             0x26
#define InqSlaveMoudle           0x27
#define InqSynAction             0x28
#define CmdDisableSynAction      0x29
#define CmdCompeteMaster         0x2A
#define CmdSlave2Master          0x2B
#define CmdMultiCheck            0x2C
#define CmdSystemSet             0x2D
#define CmdMasterTurnOnCheckOk   0x2F

#define CmdToGrided              0x30       //娑擄拷鐠у嘲鑻熺純锟�
#define CmdToIslanded            0x31       //娑擄拷鐠ч顬囩純锟�
#define CmdCloseRly              0x32       //娑擄拷鐠у嘲鎯涢崥鍫㈡埛閻㈤潧娅�
#define CmdTurnOffRly            0x33       //娑擄拷鐠у嘲鍙ч梻锟�

#define ReplyNewMsg              0x36
#define ReplySlaveMsg            0x37
#define ReplySynAction           0x38
#define ReplyUID                 0x3A
//#define ReplyDeltaTheta          0x3B
#define UserDebugData            0x3C


//#define Cmd3PhaseLoad            0x40
//#define Cmd3PhaseVolt            0x41
//#define CmdAlarmInfo             0x42
//#define CmdModuleNum             0x43







#define cSetTypeInputVoltRange              0x0001
#define cSetTypeOutputVoltage               0x0008
#define cSetTypeOutputFreq                  0x0010
#define cSetTypeRunningMode                 0x0100
#define cSetTypeOutputPF                    0x0200
#define cSetTypeStartFlag                   0x0400
#define cSetTypeTurnOff                     0x0800
#define cSetAllType                         0xFFFF


void sSendMasterData(void);
void sSendActivePower(void);
void sSendReactivePower(void);
void sSendDcCurrent(void);
void sSendCompeteData(void);
bool sSendRlySynCmd(void);

void sCANParsingForMasterModule(void);
void sCANParsingForSlaveModule(void);
void sCANParsingForNewModule(void);
void sInquiryNewModule(void);
void sInquirySlaveModule(void);
void sHandInSyn(void);
void sMasterCheckSystemStatus(void);
void sMasterInNetDetect(void);
bool sMasterCompetition(void);
void sInitUIDTable();
void sSendSystemSetting(Uint16 uwSynSetSelect,Uint16 uwSendModule);
bool sEnableParallel(void);
char sCheckMasterToSlave (void);
Uint16 ChkNewFlagLocal(void);
Uint16 Para_CheckInvRly(void);
Uint16 CheckGridRly(void);
#endif /* MODULE_PARAPROTOCOL_H_ */

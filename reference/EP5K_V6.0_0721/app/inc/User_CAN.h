//###########################################################################
#ifndef User_can_H
#define User_can_H

#ifdef __cplusplus
extern "C" {
#endif
//***************************************************************************

struct CanTx_struct
{
	Uint16 CH1;
	Uint16 CH2;
	Uint16 CH3;
	Uint16 CH4;
	Uint16 CH5;
	Uint16 CH6;
	Uint16 CH7;
	Uint16 CH8;
	Uint16 CH9;
	Uint16 CH10;
	Uint16 CH11;
	Uint16 CH12;
	Uint32 CANIF2CMD;
};
extern struct CanTx_struct Cantx;

struct Var_set
{
    Uint16 Max;     //设定值上限
    Uint16 Var;     //设定变量值
};

struct  CanRx_struct
{
    struct Var_set CH1;
    struct Var_set CH2;
    struct Var_set CH3;
    struct Var_set CH4;
    struct Var_set CH5;
    struct Var_set CH6;
    struct Var_set CH7;
    struct Var_set CH8;
    struct Var_set CH9;
    struct Var_set CH10;
    struct Var_set CH11;
    struct Var_set CH12;
    Uint32 CANIF1CMD;
};
extern struct CanRx_struct Canrx;

void User_Can(void);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif 

//===========================================================================
// No more.
//=====================================================================












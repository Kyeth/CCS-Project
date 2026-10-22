/**
 * @file User_ClaTaskMsgRam.c
 * @author yetingheng (004182)
 * @brief CPU 与 CLA 之间的消息 RAM 定义
 * @version 0.1
 * @date 2026-10-24
 * @copyright Copyright (c) 2026
 *
 * @note 为什么这两个结构体放在 CPU 侧的 .c 里定义，而不是放在 .cla 里：
 *       CLA 编译器 --cla_support 会把数据按 "blocked" 模型标注（CLA 用 16 位地址
 *       访问数据，要求对象整块可寻址），而 CPU 侧普通编译单元按 "non-blocked"
 *       标注。若定义在 .cla 里、引用在 CPU 代码里，链接期就会报
 *       #10462-D "assumes that data is blocked but is accessing non-blocked data"。
 *       把定义挪到 CPU 侧、.cla 只做 extern 引用，两侧模型即一致。
 *
 * @note 段归属必须是链接命令文件里 CLA 消息 RAM 的两个窗口：
 *       Cla1ToCpuMsgRAM → CLA1_MSGRAMHIGH (0x1500, CLA 写 / CPU 读)
 *       CpuToCla1MsgRAM → CLA1_MSGRAMLOW  (0x1480, CPU 写 / CLA 读)
 */
#include "User_ClaTask.h"

#pragma DATA_SECTION(Cla1ToCpuMsg, "Cla1ToCpuMsgRAM")
#pragma DATA_SECTION(CpuToCla1Msg, "CpuToCla1MsgRAM")

/* CLA 写、CPU 读：采样值 + 滤波结果 + 序号 */
ST_CLA_TO_CPU Cla1ToCpuMsg;

/* CPU 写、CLA 读：当前 MUX 地址、使能、角度、电流零漂 */
ST_CPU_TO_CLA CpuToCla1Msg;

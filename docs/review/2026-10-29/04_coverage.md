# 逐文件覆盖清单

基线包含 93 个文件：自研实现/头文件、主程序、两个CMD及三个CCS工程文件。逐项路径与SHA256见 source_manifest.csv。头文件与对应实现一起检查结构布局、枚举、宏、接口契约；表项对照其消费者。

这是一轮全范围静态审阅与构建验证，不能穷尽所有硬件运行状态；没有宣称逐行形式化证明。未发现问题的文件不等于已完成实板验收。

## 自研与集成文件

| 文件 | 行数 | 审阅内容 / 关联结论 |
|---|---:|---|
| [.ccsproject](<Z:/Git1029/CCS-Project/.ccsproject>) | 16 | CCS目标/编译器/ABI/宏/链接与源文件选择；F32。 |
| [.cproject](<Z:/Git1029/CCS-Project/.cproject>) | 202 | CCS目标/编译器/ABI/宏/链接与源文件选择；F32。 |
| [.project](<Z:/Git1029/CCS-Project/.project>) | 27 | CCS目标/编译器/ABI/宏/链接与源文件选择；F32。 |
| [Broad/inc/DrvCfg_EP15K.h](<Z:/Git1029/CCS-Project/Broad/inc/DrvCfg_EP15K.h>) | 87 | 全部资源表与消费方式对照；F01–F10、F13、F17、F26；GPIO接线按用户输入接受。 |
| [Broad/inc/DrvCfg_Protect.h](<Z:/Git1029/CCS-Project/Broad/inc/DrvCfg_Protect.h>) | 226 | 故障配置/协议位/动作与判据覆盖；F25，未确定的阈值不宣称合格。 |
| [Broad/src/DrvCfg_EP15K.c](<Z:/Git1029/CCS-Project/Broad/src/DrvCfg_EP15K.c>) | 770 | 全部资源表与消费方式对照；F01–F10、F13、F17、F26；GPIO接线按用户输入接受。 |
| [Broad/src/DrvCfg_Protect.c](<Z:/Git1029/CCS-Project/Broad/src/DrvCfg_Protect.c>) | 139 | 故障配置/协议位/动作与判据覆盖；F25，未确定的阈值不宣称合格。 |
| [CMD/2837xS_FLASH_APP_CLA_lnk.cmd](<Z:/Git1029/CCS-Project/CMD/2837xS_FLASH_APP_CLA_lnk.cmd>) | 206 | 程序/数据/CLA/消息RAM/入口与容量单位；F05、F32。 |
| [CMD/F2837xS_Headers_nonBIOS.cmd](<Z:/Git1029/CCS-Project/CMD/F2837xS_Headers_nonBIOS.cmd>) | 248 | 外设寄存器段地址与头文件匹配交叉检查；不视为自研外设实现。 |
| [Drivers/inc/Hw_AdcDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_AdcDriver.h>) | 184 | SOC布局/寄存器字段/时钟/触发/中断/优先级；F04、F06、F26。 |
| [Drivers/inc/Hw_CanDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_CanDriver.h>) | 99 | 位时序/对象/标准与扩展ID/IF命令/收发；F17；排除TXRQST误报。 |
| [Drivers/inc/Hw_ClaDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_ClaDriver.h>) | 142 | 程序复制、LS权限、消息RAM初始化、MVECT、触发与错误路径；F05、F31。 |
| [Drivers/inc/Hw_CpuTimerDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_CpuTimerDriver.h>) | 119 | 时钟/计数/100us节拍/自由运行和标志；供F18、F33验证；调试FREE/SOFT注释需与实际模式一致。 |
| [Drivers/inc/Hw_EcapDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_EcapDriver.h>) | 105 | 输入选择/捕获事件/周期换算/首次值；F30。 |
| [Drivers/inc/Hw_ExitDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_ExitDriver.h>) | 88 | 输入映射/边沿/使能/事件累加与取走；F30。 |
| [Drivers/inc/Hw_GpioDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_GpioDriver.h>) | 241 | 端口/GMUX/MUX/方向/开漏/初值的配置调用；未重做用户已确认的PCB引脚分配。 |
| [Drivers/inc/Hw_I2cDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_I2cDriver.h>) | 152 | 位操作/ACK/起停/延时/恢复边界；F29，实际时序待测。 |
| [Drivers/inc/Hw_InvDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_InvDriver.h>) | 83 | 相映射、占空比、换相、停止与Trip；F09、F11、F12、F31。 |
| [Drivers/inc/Hw_PwmDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_PwmDriver.h>) | 261 | 全部PWM类型初始化、同步、AQ/DB、装载、TZ、启停；F03、F08–F11、F26。 |
| [Drivers/inc/Hw_SciDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_SciDriver.h>) | 144 | 初始化/FIFO/ISR/TX/RX/帧边界/错误恢复；F13–F15。 |
| [Drivers/inc/Hw_SpiDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_SpiDriver.h>) | 157 | 时钟/格式/片选/传输/接收对齐/超时；F16；外部协议仍待实现。 |
| [Drivers/inc/Hw_XbarDriver.h](<Z:/Git1029/CCS-Project/Drivers/inc/Hw_XbarDriver.h>) | 130 | INPUT与TRIP配置、MUX源选择/使能/反相；F01、F02。 |
| [Drivers/src/Hw_AdcDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_AdcDriver.c>) | 341 | SOC布局/寄存器字段/时钟/触发/中断/优先级；F04、F06、F26。 |
| [Drivers/src/Hw_CanDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_CanDriver.c>) | 678 | 位时序/对象/标准与扩展ID/IF命令/收发；F17；排除TXRQST误报。 |
| [Drivers/src/Hw_ClaDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_ClaDriver.c>) | 280 | 程序复制、LS权限、消息RAM初始化、MVECT、触发与错误路径；F05、F31。 |
| [Drivers/src/Hw_CpuTimerDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_CpuTimerDriver.c>) | 188 | 时钟/计数/100us节拍/自由运行和标志；供F18、F33验证；调试FREE/SOFT注释需与实际模式一致。 |
| [Drivers/src/Hw_EcapDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_EcapDriver.c>) | 271 | 输入选择/捕获事件/周期换算/首次值；F30。 |
| [Drivers/src/Hw_ExitDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_ExitDriver.c>) | 173 | 输入映射/边沿/使能/事件累加与取走；F30。 |
| [Drivers/src/Hw_GpioDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_GpioDriver.c>) | 305 | 端口/GMUX/MUX/方向/开漏/初值的配置调用；未重做用户已确认的PCB引脚分配。 |
| [Drivers/src/Hw_I2cDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_I2cDriver.c>) | 413 | 位操作/ACK/起停/延时/恢复边界；F29，实际时序待测。 |
| [Drivers/src/Hw_InvDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_InvDriver.c>) | 241 | 相映射、占空比、换相、停止与Trip；F09、F11、F12、F31。 |
| [Drivers/src/Hw_PwmDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_PwmDriver.c>) | 549 | 全部PWM类型初始化、同步、AQ/DB、装载、TZ、启停；F03、F08–F11、F26。 |
| [Drivers/src/Hw_SciDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_SciDriver.c>) | 792 | 初始化/FIFO/ISR/TX/RX/帧边界/错误恢复；F13–F15。 |
| [Drivers/src/Hw_SpiDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_SpiDriver.c>) | 475 | 时钟/格式/片选/传输/接收对齐/超时；F16；外部协议仍待实现。 |
| [Drivers/src/Hw_XbarDriver.c](<Z:/Git1029/CCS-Project/Drivers/src/Hw_XbarDriver.c>) | 185 | INPUT与TRIP配置、MUX源选择/使能/反相；F01、F02。 |
| [main.c](<Z:/Git1029/CCS-Project/main.c>) | 203 | 全部初始化与任务创建、启动顺序；F10、F18。 |
| [Platform/Common/inc/AtsCtl.h](<Z:/Git1029/CCS-Project/Platform/Common/inc/AtsCtl.h>) | 76 | 反馈去抖、动作翻转、超时；应拒绝忙中重复翻转请求，当前上层命令集成未完成。 |
| [Platform/Common/inc/AtsProto.h](<Z:/Git1029/CCS-Project/Platform/Common/inc/AtsProto.h>) | 72 | 旁路监听、CRC、pending请求与响应字段解析/失联；SCIC实际对端协议需冻结。 |
| [Platform/Common/inc/Crc16.h](<Z:/Git1029/CCS-Project/Platform/Common/inc/Crc16.h>) | 34 | 表、增量计算、附加和检查；提取表与位算法一致，123456789=4B37。 |
| [Platform/Common/inc/EepromParam.h](<Z:/Git1029/CCS-Project/Platform/Common/inc/EepromParam.h>) | 230 | 点表扫描/序列化/CRC/dirty/分片写/复位；F27、F28，反向依赖应用点表。 |
| [Platform/Common/inc/Fm24c128d.h](<Z:/Git1029/CCS-Project/Platform/Common/inc/Fm24c128d.h>) | 157 | 地址/容量/WP/自检/同步和异步读写；自检位于保留区，时序依赖F29。 |
| [Platform/Common/inc/RingBuff.h](<Z:/Git1029/CCS-Project/Platform/Common/inc/RingBuff.h>) | 71 | 空满/长度/读写/清空/覆盖写接口；常规SPSC与覆盖写所有权不同，SCI清空用法见F15。 |
| [Platform/Common/src/AtsCtl.c](<Z:/Git1029/CCS-Project/Platform/Common/src/AtsCtl.c>) | 139 | 反馈去抖、动作翻转、超时；应拒绝忙中重复翻转请求，当前上层命令集成未完成。 |
| [Platform/Common/src/AtsProto.c](<Z:/Git1029/CCS-Project/Platform/Common/src/AtsProto.c>) | 325 | 旁路监听、CRC、pending请求与响应字段解析/失联；SCIC实际对端协议需冻结。 |
| [Platform/Common/src/Crc16.c](<Z:/Git1029/CCS-Project/Platform/Common/src/Crc16.c>) | 101 | 表、增量计算、附加和检查；提取表与位算法一致，123456789=4B37。 |
| [Platform/Common/src/EepromParam.c](<Z:/Git1029/CCS-Project/Platform/Common/src/EepromParam.c>) | 948 | 点表扫描/序列化/CRC/dirty/分片写/复位；F27、F28，反向依赖应用点表。 |
| [Platform/Common/src/Fm24c128d.c](<Z:/Git1029/CCS-Project/Platform/Common/src/Fm24c128d.c>) | 505 | 地址/容量/WP/自检/同步和异步读写；自检位于保留区，时序依赖F29。 |
| [Platform/Common/src/RingBuff.c](<Z:/Git1029/CCS-Project/Platform/Common/src/RingBuff.c>) | 218 | 空满/长度/读写/清空/覆盖写接口；常规SPSC与覆盖写所有权不同，SCI清空用法见F15。 |
| [User_App/Comm/inc/ArmPointTable.h](<Z:/Git1029/CCS-Project/User_App/Comm/inc/ArmPointTable.h>) | 495 | 399项地址/宽度/权限与镜像绑定；静态检查无逆序或重叠；命令生效见F24。 |
| [User_App/Comm/inc/CommRegTable.h](<Z:/Git1029/CCS-Project/User_App/Comm/inc/CommRegTable.h>) | 65 | PC111项、TVS1项/属性/32位跨度；无逆序重叠；F34、F35。 |
| [User_App/Comm/inc/DataExchange.h](<Z:/Git1029/CCS-Project/User_App/Comm/inc/DataExchange.h>) | 37 | 命令与遥测镜像映射；Rx空、发布占位；F24。 |
| [User_App/Comm/inc/ModbusSlave.h](<Z:/Git1029/CCS-Project/User_App/Comm/inc/ModbusSlave.h>) | 127 | 地址查找/03/06/10/长度/CRC/广播/读写类型/保存标记；F15、F34、F35；未实现04。 |
| [User_App/Comm/inc/SoftOsc.h](<Z:/Git1029/CCS-Project/User_App/Comm/inc/SoftOsc.h>) | 183 | 通道绑定/采样缓存/分组CAN发送/频率；需要邮箱忙和丢帧监测，调度压力见F33。 |
| [User_App/Comm/src/ArmPointTable.c](<Z:/Git1029/CCS-Project/User_App/Comm/src/ArmPointTable.c>) | 439 | 399项地址/宽度/权限与镜像绑定；静态检查无逆序或重叠；命令生效见F24。 |
| [User_App/Comm/src/CommRegTable.c](<Z:/Git1029/CCS-Project/User_App/Comm/src/CommRegTable.c>) | 222 | PC111项、TVS1项/属性/32位跨度；无逆序重叠；F34、F35。 |
| [User_App/Comm/src/DataExchange.c](<Z:/Git1029/CCS-Project/User_App/Comm/src/DataExchange.c>) | 234 | 命令与遥测镜像映射；Rx空、发布占位；F24。 |
| [User_App/Comm/src/ModbusSlave.c](<Z:/Git1029/CCS-Project/User_App/Comm/src/ModbusSlave.c>) | 675 | 地址查找/03/06/10/长度/CRC/广播/读写类型/保存标记；F15、F34、F35；未实现04。 |
| [User_App/Comm/src/SoftOsc.c](<Z:/Git1029/CCS-Project/User_App/Comm/src/SoftOsc.c>) | 497 | 通道绑定/采样缓存/分组CAN发送/频率；需要邮箱忙和丢帧监测，调度压力见F33。 |
| [User_App/InterruptCfg/inc/InterruptCfg.h](<Z:/Git1029/CCS-Project/User_App/InterruptCfg/inc/InterruptCfg.h>) | 56 | 向量/PIE/核心Timer/CLA完成/SCI/XINT/喂狗；F10、F18；MICLR不是完成中断应答的通用替代。 |
| [User_App/InterruptCfg/src/InterruptCfg.c](<Z:/Git1029/CCS-Project/User_App/InterruptCfg/src/InterruptCfg.c>) | 192 | 向量/PIE/核心Timer/CLA完成/SCI/XINT/喂狗；F10、F18；MICLR不是完成中断应答的通用替代。 |
| [User_App/LoopCtrl/inc/AdcCalib.h](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/inc/AdcCalib.h>) | 254 | 标称增益/零点/零漂窗口/工程值转换/启动有效性；依赖F04–F07；实物标定仍未测。 |
| [User_App/LoopCtrl/inc/AdcSample.h](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/inc/AdcSample.h>) | 84 | 瞬时/RMS/多时间尺度平均/功率与发布；F26、F33；输入有效与一致性必须补齐。 |
| [User_App/LoopCtrl/inc/CtrlIsr.h](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/inc/CtrlIsr.h>) | 192 | 快环执行、PLL、授权、半周、MUX、状态分频、故障和余量；F10–F12、F18–F22、F25、F33。 |
| [User_App/LoopCtrl/inc/ProtectFault.h](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/inc/ProtectFault.h>) | 62 | 故障等级/去抖/释放/锁存/协议聚合；判据和执行覆盖见F25。 |
| [User_App/LoopCtrl/inc/ProtectInput.h](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/inc/ProtectInput.h>) | 50 | 实际软件判据生产者与时间单位；F25。 |
| [User_App/LoopCtrl/inc/ProtectTask.h](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/inc/ProtectTask.h>) | 110 | 硬件事件计数/CBC/锁存/恢复与封波；F01–F03、F18、F22、F25。 |
| [User_App/LoopCtrl/inc/SampleData.h](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/inc/SampleData.h>) | 152 | 共享采样模型/单位/消费者；数据一致性与有效性见F33。 |
| [User_App/LoopCtrl/inc/User_ClaTask.h](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/inc/User_ClaTask.h>) | 184 | Task1/2全部采样结果、滤波、MUX归档、共享接口；F05–F07、F26。 |
| [User_App/LoopCtrl/src/AdcCalib.c](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/AdcCalib.c>) | 392 | 标称增益/零点/零漂窗口/工程值转换/启动有效性；依赖F04–F07；实物标定仍未测。 |
| [User_App/LoopCtrl/src/AdcSample.c](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/AdcSample.c>) | 379 | 瞬时/RMS/多时间尺度平均/功率与发布；F26、F33；输入有效与一致性必须补齐。 |
| [User_App/LoopCtrl/src/CtrlIsr.c](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/CtrlIsr.c>) | 714 | 快环执行、PLL、授权、半周、MUX、状态分频、故障和余量；F10–F12、F18–F22、F25、F33。 |
| [User_App/LoopCtrl/src/ProtectFault.c](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/ProtectFault.c>) | 265 | 故障等级/去抖/释放/锁存/协议聚合；判据和执行覆盖见F25。 |
| [User_App/LoopCtrl/src/ProtectInput.c](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/ProtectInput.c>) | 124 | 实际软件判据生产者与时间单位；F25。 |
| [User_App/LoopCtrl/src/ProtectTask.c](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/ProtectTask.c>) | 265 | 硬件事件计数/CBC/锁存/恢复与封波；F01–F03、F18、F22、F25。 |
| [User_App/LoopCtrl/src/SampleData.c](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/SampleData.c>) | 35 | 共享采样模型/单位/消费者；数据一致性与有效性见F33。 |
| [User_App/LoopCtrl/src/User_ClaTask.cla](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/User_ClaTask.cla>) | 234 | Task1/2全部采样结果、滤波、MUX归档、共享接口；F05–F07、F26。 |
| [User_App/LoopCtrl/src/User_ClaTaskMsgRam.c](<Z:/Git1029/CCS-Project/User_App/LoopCtrl/src/User_ClaTaskMsgRam.c>) | 30 | 共享结构实例和链接段；F05。 |
| [User_App/Schedule/inc/Schedule.h](<Z:/Git1029/CCS-Project/User_App/Schedule/inc/Schedule.h>) | 70 | 合作式任务节拍/到期/欠拍/健康窗；独立监控与最坏时延见F18、F33。 |
| [User_App/Schedule/src/Schedule.c](<Z:/Git1029/CCS-Project/User_App/Schedule/src/Schedule.c>) | 156 | 合作式任务节拍/到期/欠拍/健康窗；独立监控与最坏时延见F18、F33。 |
| [User_App/StateMachine/inc/BusStateMachine.h](<Z:/Git1029/CCS-Project/User_App/StateMachine/inc/BusStateMachine.h>) | 64 | 压差滤波/初始化/工作/错误；PI与PWM输出未实现，F03、F19。 |
| [User_App/StateMachine/inc/InvStateMachine.h](<Z:/Git1029/CCS-Project/User_App/StateMachine/inc/InvStateMachine.h>) | 110 | 建压/自检/幅值调节/合闸/运行/故障；F19–F22。 |
| [User_App/StateMachine/inc/PvStateMachine.h](<Z:/Git1029/CCS-Project/User_App/StateMachine/inc/PvStateMachine.h>) | 168 | 四路状态、限幅/滤波/环路/MPPT/重试/下发；F23，功率闭环未验证。 |
| [User_App/StateMachine/inc/RelayCtl.h](<Z:/Git1029/CCS-Project/User_App/StateMachine/inc/RelayCtl.h>) | 82 | 继电器统一写入和状态缓存；合闸许可在上层不足见F20，GPIO回读不能证明触点状态。 |
| [User_App/StateMachine/inc/SmUtil.h](<Z:/Git1029/CCS-Project/User_App/StateMachine/inc/SmUtil.h>) | 108 | 限幅/斜坡/计数等辅助；未发现独立阻断项，调用周期和非有限输入仍需契约。 |
| [User_App/StateMachine/inc/SystemStateMachine.h](<Z:/Git1029/CCS-Project/User_App/StateMachine/inc/SystemStateMachine.h>) | 99 | 上电/启动/运行/停机/故障/恢复；F22、F24。 |
| [User_App/StateMachine/src/BusStateMachine.c](<Z:/Git1029/CCS-Project/User_App/StateMachine/src/BusStateMachine.c>) | 103 | 压差滤波/初始化/工作/错误；PI与PWM输出未实现，F03、F19。 |
| [User_App/StateMachine/src/InvStateMachine.c](<Z:/Git1029/CCS-Project/User_App/StateMachine/src/InvStateMachine.c>) | 329 | 建压/自检/幅值调节/合闸/运行/故障；F19–F22。 |
| [User_App/StateMachine/src/PvStateMachine.c](<Z:/Git1029/CCS-Project/User_App/StateMachine/src/PvStateMachine.c>) | 644 | 四路状态、限幅/滤波/环路/MPPT/重试/下发；F23，功率闭环未验证。 |
| [User_App/StateMachine/src/RelayCtl.c](<Z:/Git1029/CCS-Project/User_App/StateMachine/src/RelayCtl.c>) | 106 | 继电器统一写入和状态缓存；合闸许可在上层不足见F20，GPIO回读不能证明触点状态。 |
| [User_App/StateMachine/src/SmUtil.c](<Z:/Git1029/CCS-Project/User_App/StateMachine/src/SmUtil.c>) | 111 | 限幅/斜坡/计数等辅助；未发现独立阻断项，调用周期和非有限输入仍需契约。 |
| [User_App/StateMachine/src/SystemStateMachine.c](<Z:/Git1029/CCS-Project/User_App/StateMachine/src/SystemStateMachine.c>) | 255 | 上电/启动/运行/停机/故障/恢复；F22、F24。 |
| [User_App/WatchDog/inc/Hw_WatchDog.h](<Z:/Git1029/CCS-Project/User_App/WatchDog/inc/Hw_WatchDog.h>) | 43 | 时钟公式/初始化/喂狗/复位；F18。 |
| [User_App/WatchDog/src/Hw_WatchDog.c](<Z:/Git1029/CCS-Project/User_App/WatchDog/src/Hw_WatchDog.c>) | 102 | 时钟公式/初始化/喂狗/复位；F18。 |

## 第三方、生成文件与非运行内容

- device/common/source：检查当前调用的InitSysCtrl、ADC trim、PIE初始化、CodeStart/Delay，以及链接/编译集成；通过强制构建验证全部参与构建的厂商源文件。没有把未调用的TI示例/USB/EMIF等逐行认证为正确。
- device/headers/include 与 driverlib：重点交叉检查EPWM_REGS保留地址、SYNCSELECT、ADC/CAN/CLA/SCI/SPI/X-BAR寄存器位定义和官方枚举；厂商源文件不是本次改写目标。
- Debug：检查makefile、编译参数、链接输入及独立副本生成的map；已有obj/out/d文件是产物，不能替代源码正确性证据。未覆盖用户原有Debug文件。
- targetConfigs / CCS工程：未连接探针验证目标器件/Boot模式；硬件连接和下载运行保持未验证。
- 历史备份、截图、二进制/旧代码不作为当前运行路径；若存在独立Bootloader或未纳入工程的板级程序，需要另行纳入集成验证。

## 可复现静态检查

从项目根目录执行 `python docs/review/2026-10-29/evidence/audit_checks.py`。脚本只生成审阅证据，不写固件；要求保留 tmp/audit 中本次构建日志/map副本用于归档。它重放当前ADC配置访问、检查点表顺序/宽度及CRC，输出频率/窗口/看门狗/滤波计算。它不是C28x模拟器，也不能覆盖全部控制/并发缺陷。

## 构建证据说明

build.log记录全量编译及首次缺库的副本链接失败；link.log记录复制四个原工程driverlib库后再次链接成功。原始build.py一并归档，以保留当时步骤；若重新复现，应先将device/driverlib/ccs/Debug和Release所需.lib复制进副本，或按已批准构建方案收敛库依赖。复制工程的include路径仍指向当前工作区同一份头文件，因此该构建验证的是当前源码内容，不能视为脱离本机路径的发布构建。

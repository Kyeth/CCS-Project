# TMS320F28374S PCS 代码审阅（2026-10-29）

**结论：当前工程能够编译、链接，但不能按描述的控制链路和功率级设计正常运行。** 已发现足以独立阻断采样、CLA 数据发布、通信和硬件保护的配置错误。当前不能将“CCS 下载成功”“某一路 PWM 有波形”视为底层验收通过。

> 本目录的缺陷编号、行号与 evidence 是修改前的审阅基线。后续修复进展和当前仍未实现的功能见 [修复状态与下一步验证](05_fix_status.md)。

交付内容：

1. [当前代码逻辑与设计对应关系](01_current_logic.md)：启动、PWM/ADC/CLA/ISR、通信、保护、状态机与分层。
2. [缺陷与漏洞文档](02_defects.md)：按严重性列出证据、影响、修复方向与验收方法。这里的“漏洞”主要指功能、时序和保护缺陷，不是网络攻击漏洞。
3. [底层重构边界与验证方案](03_validation_plan.md)：按 Broad → Drivers → Platform → 最小 User_App 落地，包含寄存器检查和台架测试矩阵。
4. [逐文件覆盖清单](04_coverage.md)：自研文件、头文件、链接/工程配置的覆盖范围与限制。
5. [配置重放结果](evidence/audit_results.json)、[64 项 ADC 对照](evidence/adc_register_replay.csv)、[审阅源文件指纹](evidence/source_manifest.csv)、[链接 MAP](evidence/EP15K_2026.map)、[编译日志](evidence/build.log)、[链接日志](evidence/link.log)。

## 本次实际做了什么

- 阅读自研 Broad、Drivers、Platform、User_App 的实现与接口，追踪主程序、初始化顺序、中断、状态机、通信和持久化的调用关系；检查本次构建使用的 CMD 和工程参数。
- 对照用户提供的 SPRUHX5I（2024-05）及 TI F2837xS 数据手册 SPRS881K（2024-02），复核涉及的寄存器和存储器规则。
- 在 `tmp/audit/buildcheck` 独立副本中强制重新编译 C、CLA、汇编并链接，使用本机 TI C2000 22.6.4.LTS，`--abi=eabi --cla_support=cla1 --define=F2837xS --define=_FLASH`。编译和最终链接通过；见日志。第一次链接由于审阅副本未复制 driverlib 预编译库而中断，补齐副本依赖后链接通过，这不是原项目缺陷。
- 静态重放 ADC 表访问，64 项中 **54 项与表作者意图不符**；期望的 4 项 SOCB 配置实际一项也未读到。计算频率、死区、采样窗、滤波和看门狗时间；复核 CRC 表及标准向量。
- 保留原有固件和 User_App，没有删除应用、改动驱动、覆盖原 Debug 构建结果或回退已有未提交改动。本次新增审阅文档和审阅用临时证据。后续可根据文档将 User_App 替换为最小底层验证程序。

## 验证边界

本次没有连接 JTAG/实际控制板，没有测量门极、采样、电流、通信收发器或保护传播延迟；静态重放不是 DSP 仿真。不能确认 15 kW 运行、闭环稳定性、器件死区裕量或保护阈值。用户确认的 GPIO 对应关系作为输入条件接受；仍检查其后级 X-BAR/PWM/ADC 等配置。

用户提供的 T 型单极性开关逻辑在逻辑层面能够由这些 ePWM 资源表达；当前代码未完整实现该行为。离网/并网闭环方案仍属设计候选，缺少 L/C、采样增益实测、驱动延迟、母线范围等参数，不能据现有代码判定控制稳定性。

## 手册与引用约定

- TRM：`C:/Users/001/Desktop/工作/spruhx5i.pdf`。缺陷文档中的页码是该 PDF 内页码；“章节/寄存器名”用于跨修订定位。
- [TI F2837xS 数据手册 SPRS881K](https://www.ti.com/lit/ds/symlink/tms320f28374s.pdf)：表 7-3，p.187；消息 RAM 访问规则 p.195。CLA→CPU 为 0x1480，CPU→CLA 为 0x1500。
- 工程内 `device/headers/include` 和 `device/driverlib` 用于交叉检查，不把注释或文档中的指令当作用户操作要求。
- 缺陷位置为审阅时源文件行号，指纹见清单。修复后行号可能变化。

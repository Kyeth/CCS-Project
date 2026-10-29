# 缺陷与漏洞文档

审阅基线：2026-10-29，源文件 SHA256 见 evidence/source_manifest.csv。本文中的路径相对项目根目录。P0 表示功率输出/保护存在严重风险，必须先于发波修复；P1 表示核心功能阻断或结果错误；P2 表示可靠性、接口或维护缺陷。标注“确定”指能从源代码与硬件规则确认，不表示已在实板复现。“待实测”不能当作已确认故障。

## 一、必须先解决的配置和保护问题

### F01 · P0 · X-BAR MUX 编号不是 INPUT 编号减一【确定】

- 位置：`Broad/src/DrvCfg_EP15K.c:720`；`Drivers/src/Hw_XbarDriver.c:105`。
- 证据：当前 INPUT1/2/3/6/4/5 对应 MUX0/1/2/5/3/4。TRM Table 8-2，p.1113，INPUT1…6 应选 **MUX1/3/5/7/9/11 的源选择值1**。例如 MUX0=1 选择 CMPSS1 的组合信号，并非 INPUT1；当前 TRIP5 的 MUX1 实际接 INPUT1，并非 PV2。
- 影响：PV 保护错路、漏路，INV TRIP 来源错误，不能按接线图保护目标开关。
- 修复：表结构分开 `mux_index`、`source_select`、`enable`；使用本器件枚举或 driverlib 常量，禁止“DISABLE/ENABLE/INVERT”三值替代两位源选择。源值0也是合法输入，不能自动解释成禁止。
- 验收：驱动禁止状态下逐一注入六路故障，读 INPUT、TRIP 和 TZ 标志并测对应输出；一项一项验证，不只测试同时故障。

### F02 · P0 · 低有效过流/短路先 OR 后检测低，单故障被正常信号遮蔽【确定】

- 位置：`Broad/src/DrvCfg_EP15K.c:680`、`:727`、`:732`。
- 即使先修正 F01，TRIP9 原样 OR 两路低有效信号，DCA/DCB 均检测 LOW。过流=0、短路=1 时 OR=1，不触发；反之亦然；只有两路同时0才触发。TRM p.1112 明确启用的 MUX 输出逻辑 OR。
- 修复：两路分别进入可独立触发的 DC/TZ 路径，或先各自正确归一化为高有效再 OR；仅对合成结果整体取反无法修复单故障遮蔽。同步明确 CBC/OST 策略。
- 验收：四行真值表逐项测试，见 audit_results.json。CPU 停止服务时硬件仍必须独立动作。

### F03 · P0 · BUS 输出没有对应硬件 TZ 保护闭环【确定】

- 位置：`Broad/src/DrvCfg_EP15K.c:677` 的 TZ 表仅有 INV 六路、PV 四路；X-BAR 表未含 BUS 过流/母线过压输入。
- 影响：EPWM8 没有与 INV/PV 等价的配置保护；GPIO77/80 的存在不等于已能关闭 BUS 两路输出。
- 修复：按硬件极性和故障作用域补 BUS 双输出安全动作、过压联动、故障锁存/人工恢复；短路是否立即 OST、普通限流是否 CBC 要作为显式策略，而非统一 EVT2 默认行为。
- 验收：每个故障分别验证 INV/PV/BUS 的动作矩阵和传播延迟；DCACTL/DCBCTL 同步/异步选择和最小可捕获脉宽须有明确配置及测量。

### F04 · P1 · ADC 配置表 stride 不匹配【确定】

- 位置：`Broad/src/DrvCfg_EP15K.c:355`、`:390`；`Drivers/src/Hw_AdcDriver.c:205`。
- 表为交织 A/B/C/D，驱动却将每个指针当作连续16项。重放结果54/64项错误，四个预期 SOCB 项全部丢失。A15实际采通道3，A9实际由SOCA采通道1。
- 影响：电压/电流/保护使用错误物理量，第二采样不运行；CLA 两任务的触发批次错误。可能仍有中断，不应误诊为 SOC15 未配置。
- 修复：推荐每个 ADC 持有自己的16项完整配置（二维 `[ADC][SOC]`）；显式标明 unused，并在初始化重入时清除旧配置，不要只跳过。
- 验收：64项寄存器读回逐字段对照；逐通道输入不同可辨识电压，检查 RESULT 与业务名一致。

### F05 · P1 · CLA 双向消息 RAM 放反【确定】

- 位置：`CMD/2837xS_FLASH_APP_CLA_lnk.cmd:185`；`User_App/LoopCtrl/src/User_ClaTaskMsgRam.c:17`。
- 当前 CpuToCla→0x1480，ClaToCpu→0x1500；MAP 已确认。正确方向相反：CLA→CPU 在0x1480，CPU→CLA在0x1500。数据手册 Table7-3 p.187、p.195；TRM §6.3.1 p.675 说明 CLA 向 CPU→CLA 区写入会被忽略。
- 影响：CLA 结果、序号不能按意图发布，CPU写MUX命令也不符合该区权限；调试器可写不代表运行中的CPU/CLA可写。
- 修复：交换消息段链接目标，统一代码注释、结构声明和启动初始化；检查段大小、共享结构布局和各自主写权。
- 验收：最小CLA任务读CPU token，写 response 和seq，CPU连续验证；断开调试器写内存干预后重新冷启动。

### F06 · P1 · 默认 ADC round-robin 不能保证 EOC15 是第一批完成屏障【确定，修复F04后显现】

- 位置：`Drivers/src/Hw_AdcDriver.c:160` 没有配置 SOCPRIORITY；Broad 表含SOC8/9第二批。
- TRM §11 ADC SOC Priority，p.1474：优先转换的是 RRPOINTER 后下一个待处理SOC。第二批结束指针9时，下一SOCA的SOC15可先于0…7完成。
- 影响：CLA Task1在本批数据未完成时启动，混用新旧数据；“末位编号+长采样窗”并不自动构成跨ADC屏障。
- 修复：显式配置确定的SOC优先级/排程（例如所有相关SOC固定优先顺序），计算每ADC每批的最坏完成时间，再选择Task触发。不能只加任意延时或只改INTSEL。
- 验收：至少覆盖启动第一批、SOCA/SOCB交替稳态、任务延迟、触发溢出，记录各EOC和CLA启动时刻。

### F07 · P1 · CLA Task2 读取第一采样结果寄存器【确定】

- 位置：`User_App/LoopCtrl/src/User_ClaTask.cla:54`；`User_App/LoopCtrl/inc/User_ClaTask.h` 的 CLA_RES_INV_CUR 宏。
- Task2仍读SOC0/1结果，未读A8、D8、D9。即使SOCB配置修好，也不会获得正确的PRD电流。
- 修复：独立定义第二采样结果映射；明确Task1用本周期ZERO与上周期PRD配对的时间语义，附第二样本有效位/序号；不可把两点均值无条件当作准确周期平均。
- 验收：给ZERO/PRD两个窗口可区分的信号，分别检查两组原始码和配对结果。

### F08 · P0 · ePWM 同步写入本芯片保留地址【确定】

- 位置：`Drivers/inc/Hw_PwmDriver.h:55`、`:56`；`Drivers/src/Hw_PwmDriver.c:225`。
- 用模块基址+3/+6模拟 EPWMSYNCINSEL/EPWMSYNCOUTEN；F2837xS 的 EPWM_REGS 这两个偏移是保留区，应使用本代 TBCTL.SYNCOSEL、SYNCSELECT 及固定同步链。依据 TRM ePWM寄存器汇总和同步链章节、项目 F2837xS_epwm.h。
- 影响：表里配置的同步源不生效；TBPHS不因写表就自动装入计数器；PV交错和INV六路同步不能保证。
- 修复：删除跨代寄存器偏移宏，按照 1–3 / 4–6 / 7–9 / 10–12 分组及支持的跨组同步选择设计路由。不能假设EPWM7可任意直接回送EPWM5/6。统一主时基，补偿同步传播延时并明确PHSDIR。
- 验收：示波器同时观察组边界及四路PV相位，重启/重同步后相位可重复；读回TBCTL、SYNCSELECT、TBPHS。

### F09 · P0 · 全局装载被强制关闭，换相的同步提交假设不成立【确定】

- 位置：`Drivers/src/Hw_PwmDriver.c:170`、`:248`；`Drivers/src/Hw_InvDriver.c:118`、`:159`。
- 表的bGload=1最终仍被置GLD=0，GLDCTL2LINK重写为0；CMPCTL仍SHADOW且CTR=ZERO装载。注释“全部直写立即生效”错误，Hw_TrigGlobalLoad不能使已禁用的全局装载组同步提交。GLDMODE=0本身是ZERO，不是“无脉冲”；EPWMXLINK源/目标方向注释也应按手册重写。
- 影响：CMP/AQ force与gInvDuty软件缓存不代表同一已生效周期；换半周的“上拍已归零”判断不可靠。
- 修复：明确使用各模块同相ZERO局部装载或正确配置one-shot全局装载，定义统一stage/commit及活动状态反馈。跨正负模块先钳位后释放的顺序应由明确PWM边界保障。
- 验收：过零、低占空比、最大占空比、命令紧邻ZERO到达、ISR延迟时测四门极；禁止只以寄存器影子读回证明已生效。

### F10 · P0 · 时基/中断早于状态对象初始化启动【确定】

- 位置：`Drivers/src/Hw_PwmDriver.c:297`；`main.c:92`、`:110`。
- Hw_PwmInit放开TBCLK，PeripheralsInit末尾EINT，随后才执行PlatformInit。ADC/CLA可以在初始化其消费者时并行运行。
- 修复：全程保留禁止驱动和安全TZ状态；完成所有数据、CLA、协议、保护及向量初始化后，清待决标志，在单一Start函数启动时基和中断。运行链Ready与功率Enable必须分开。
- 验收：冷启动打点证明第一次控制ISR晚于所有依赖Ready；首次样本无效时不参与工程值保护/闭环。

### F11 · P0 · AQ 中点钳位不能替代故障全关，BUS 初始化尤其需分清【确定】

- 位置：`Drivers/src/Hw_InvDriver.c:43`、`:164`、`:203`；`Drivers/src/Hw_PwmDriver.c` deadband/force 初始化；`CtrlIsr.c:426`。
- A低经过互补DB后B高，INV S2/S3常导通是中点钳位；BUS同样可能为A低/B高。一般Ctrl故障分支仅CtrlAuthBlock/InvStop，不能保证四管全关；硬件保护路径另有TripAll，行为不统一。
- 修复：分别提供 NeutralClamp、GateDisable、TripLatch；为每类故障明确策略，严重故障统一执行所有相关功率级的可靠全关与锁存。对驱动使能GPIO88建立显式生命周期，不能靠初始低状态永久代替保护。
- 验收：分别量正常零输出、软件停机、硬件故障、控制失联四种状态；明确外部驱动的实际有效极性。

### F12 · P1 · INV 初始化/Stop后同半周 duty 不会释放 force【确定】

- 位置：`Drivers/src/Hw_InvDriver.c:75`、`:111`、`:140`。
- Init把gInvChop记为正模块但输出仍强制钳位；第一次下发正半周进入“同半周无需换相”，不会ReleaseForce。Stop后恢复也可能复现。应用CtrlAuthArm通过切两次极性绕过，并未修好驱动契约。
- 修复：驱动独立维护Disabled/Clamp/Active/Commutating状态，显式Arm初始化活动状态；SetDuty不依赖调用者人为翻半周。
- 验收：Init→正/负首次Enable，Stop→同/异半周恢复均有确定波形。

## 二、通信与运行监视

### F13 · P1 · SCI 阈值8且未取尾包【确定】

- 位置：`Broad/src/DrvCfg_EP15K.c:501`附近 stSciCfgItem；`Drivers/src/Hw_SciDriver.c:599`。
- Scan只补TX和按软件计数判帧，不读硬件RX FIFO。少于8字节的帧不触发RX中断；非8整数倍长帧的尾部留FIFO，可能跨帧拼接。7字节响应、11/13字节写多寄存器可复现。
- 修复：底层收字节与协议帧边界解耦；例如RXFFIL=1并正确排空FIFO，或在可靠空闲处理路径取尾包。空闲时间须基于真实字节到达，不能基于批量搬移时刻。
- 验收：长度1…最大帧长，特别7/8/9/11/13/16/17，测试连续帧、分段发送和长静默。

### F14 · P1 · SCI FIFO 读取错误依赖 RXRDY，错误标志恢复实现错误【确定寄存器语义】

- 位置：`Drivers/src/Hw_SciDriver.c:553`、`:477`。
- FIFO循环虽取得RXFFST，却每读前检查RXRDY；TRM p.2188明确读SCIRXBUF会清RXRDY。该位不能替代FIFO剩余计数。SCIRXST错误位为只读状态，写0x003C不能按W1C清除（p.2188–2191）。
- 修复：FIFO模式以RXFFST排空并处理RXFFOVF；根据器件要求执行SWRESET/通道恢复并废弃受损帧；错误计数和恢复次数对外可见。
- 验收：一次收满8/16字节检查搬移数量；注入帧错、break、溢出后能够接收下一合法帧。实际RXRDY/FIFO波形仍需实板观察，不能以“只改RXBKINTENA”修复。

### F15 · P1 · SCI FrameRead 清整个环形缓冲，可能丢下一帧【确定可达交错】

- 位置：`Drivers/src/Hw_SciDriver.c:683`、`:640`。
- 已发布帧等待消费期间，ISR可继续写下一帧前缀；FrameRead取完本帧后RingBuffClear把后续前缀也删除。下一帧到来且上一帧未消费时，丢帧分支清ring但未同步撤销原ready描述。
- 修复：固定帧描述符/双缓冲，消费只推进已发布帧的长度；明确ISR与后台对长度、边界、ready的所有权；overflow废弃整帧并恢复边界。
- 验收：故意延迟解析任务，发送两个最小帧间隔帧，验证不拼帧、不发布旧长度、不静默丢尾。

### F16 · P1 · SPI 接收二次右移【确定】

- 位置：`Drivers/src/Hw_SpiDriver.c:204`。
- SPIRXBUF已经右对齐（TRM p.2120、接收寄存器说明）；8位模式再右移8位会丢弃有效字节，不能把发送左对齐规则用于接收。INT_FLAG由读取RXBUF清除，也不是写1清除位。
- 修复：接收只按字符位宽mask；使用明确的RX完成/溢出清理语义。SPI当前没有完整ARM协议调用路径，修复寄存器不等于通信协议完成。
- 验收：loopback/外部从机返回0x00/0x55/0xA5/0xFF，覆盖8/16位和多字节CS保持。

### F17 · P1 · CAN 标准帧 ID 未移至 ARB[28:18]【确定】

- 位置：`Drivers/src/Hw_CanDriver.c:211`、`:219`、`:440`、`:595`；Broad CANA配置bExtId=0。
- 标准帧ID/屏蔽值直接放低位，读回也没有右移18。小于0x800的ID会在标准帧真正标识字段中成为0。当前CANA是标准帧，问题并非仅潜在API风险；CANB扩展帧不受这个位移问题影响。
- 修复：按标准/扩展类型分别编码ID、mask和读取；限制标准ID≤0x7FF，扩展ID≤0x1FFFFFFF；处理发送邮箱pending，避免未发完覆盖。
- 验收：CANA发0x123并用分析仪确认ID；筛选0x123和相邻ID；CANB发扩展ID；无ACK/总线离线/邮箱忙压力测试。

### F18 · P0 · 看门狗计算漏固定 /512，控制失联后仍可能保持旧输出【确定】

- 位置：`User_App/WatchDog/src/Hw_WatchDog.c:13`、`:37`；`User_App/InterruptCfg/src/InterruptCfg.c:109`。
- TRM WDCR p.223：WDCLK=INTOSC1/512/WDPS。Hw_WatchDogInit(2)选WDPS7，实际约838.86ms，不是注释1.6384ms；加20ms宽限后，失联至复位可能约0.86s。独立Timer只停喂狗，未立即执行Trip。
- CtrlIsr内部检查CLA停摆不能发现“CLA完成中断完全不再来”的情况，因为检测函数本身也不执行。
- 修复：独立Timer监控采样/CLA/CPU序列与时间戳，超时先硬件锁存全功率级禁止，再按策略复位；修正WD公式并将请求值/实际值返回。具体超时上限由功率级响应需求决定，不能任意承诺2ms。
- 验收：分别停SOCA、屏蔽CLA任务、屏蔽PIE、卡住CPU/后台，测从故障到门极安全状态以及复位的时间。

## 三、应用层现状影响底层判断的问题

### F19 · P1 · 当前 INV 不具备目标瞬时电压/电流闭环【确定功能缺失】

- 位置：`CtrlIsr.c:442`及调制输出；`InvStateMachine.c:65`；`BusStateMachine.c:88`。
- 当前为PLL/正弦调制、R相RMS幅值积分式调节公共调制比。无用户要求的瞬时电压环/电流内环、半母线前馈、并网电流环/电网前馈、重复控制。BUS PI和EPWM8下发为TODO。
- 修复/验收：先移除自动运行流程，保留最小链路验证；后续依据L/C/延时和采样模型选择控制器并做离散稳定性、限幅/抗积分饱和、模式切换验证。不能靠补一个PI就声称符合设计。

### F20 · P0 · 并网合闸条件仅比较 R 相 RMS 幅值【确定】

- 位置：`User_App/StateMachine/src/InvStateMachine.c:235`。
- 同幅反相也满足RMS差阈值；合闸条件未同时要求相位差、频差、相序、三相有效值、PLL锁定及数据新鲜度。
- 修复：底层验证程序禁用自动并网合闸；后续建立明确同步许可和独立失网/失同步撤销机制。
- 验收：同幅180°相差、错误相序、缺相、ATS状态过期必须拒绝合闸；正常条件持续满足指定时间才许可。此项不由GPIO对应正确性消除。

### F21 · P1 · 软停机状态不再更新 PWM，斜坡只在软件变量里下降【确定】

- 位置：`CtrlIsr.c:294`、`:507`。
- SOFT_OFF降低fModActual，但下发条件只允许AUTH_ON。撤销Enable后硬件可能保持进入SOFT_OFF前的CMP，直到最终钳位，实际不是逐拍降幅。
- 修复：RUN与RAMP_DOWN都走统一有效命令提交；严重故障绕过软停，直接锁存禁止。
- 验收：示波器确认停机每拍实际占空比变化，与fModActual一致。

### F22 · P0 · 故障提前return冻结后级状态机，恢复逻辑也存在闭锁【确定】

- 位置：`CtrlIsr.c:426`；`SystemStateMachine.c:234`；`CtrlIsr.c:616`。
- 非零故障在CtrlSmSchedulerRun之前return，系统/INV/PV/BUS状态机和部分采样发布停止推进。系统FAULT复位又要求InvSmGetFault()==NONE才调用InvSmReset，INV自身已锁存故障时无法走该路径；已锁存TZ也不能单靠CtrlClearFault恢复。
- 修复：独立且始终运行的安全状态/故障处理通路，统一撤销INV/PV/BUS授权；复位顺序应验证物理原因消失→清软件故障→重新准备命令→按受控条件清TZ，不能后台任意清标志。
- 验收：控制过流、保护锁存、任务停摆分别进入安全态，恢复命令既不死锁也不带故障重发波。

### F23 · P1 · PV 缺少输出授权闭环；停机不能用CMP=1代替关断【确定】

- 位置：`PvStateMachine.c:103`、`:126`；`Drivers/src/Hw_PwmDriver.c`初始化force。
- PV状态机只写CMP，未找到解除初始AQ强制的有效调用；正常流程可能一直无波。若调试时手工解除，Stop写CMP=1只是极小脉冲，不是硬件禁止。gPvEnable在IDLE检查，运行/故障自动重试没有统一撤销路径。
- 修复：每路PV提供明确Arm/Run/Stop/Trip，独立故障锁存及整机禁止优先级；Stop输出硬禁止，不以0占空比边界行为猜测。
- 验收：四路分别启动、停机、故障、取消使能、重试，测实际门极并检验互不误触发。

### F24 · P1 · ARM 命令镜像未搬运到实际控制【确定】

- 位置：`User_App/Comm/src/DataExchange.c:57`。
- DataExchangeRx为TODO；上位机写成功可仅改变镜像，未转换成系统启动/模式/功率命令，部分发布状态也是常量占位。
- 修复：建立命令校验、请求序号、应用结果和拒绝原因；仅暴露已实现字段，未实现的写入明确返回不支持。
- 验收：每个可写点均有“接收→校验→状态迁移或拒绝→回读”完整证据。

### F25 · P1 · 保护配置项不等于已实现判据，部分时间单位错误【确定】

- 位置：`Broad/src/DrvCfg_Protect.c`；`ProtectInput.c:45`、`:75`；`CtrlIsr.c:337`。
- ProtectInput仅实现逆变输出三相及直流分量判据，大量枚举/配置无对应采样生产者。保护协议字与硬件锁存故障未全面统一。每10ms调用一次却对 `_MS` 门限每次只+1，实际持续时间约为数字×10ms；Ctrl分频先比较>=200再自增导致201拍=10.05ms。
- 修复：生成每个故障的输入源、阈值、单位、周期、去抖、锁存、作用域和复位条件表；明确未实现项。用实际elapsed时间或统一tick单位。
- 验收：时间轴注入测试验证每项触发/恢复时刻、协议位与实际输出一致；未确定阈值保持待定，不填假定合格数值。

## 四、数值、持久化和接口可靠性

### F26 · P2 · 时钟/采样窗/滤波注释与计算不一致【确定】

- 位置：`Hw_AdcDriver.c:186`及 `Hw_AdcDriver.h`；Broad ADC/PWM表；`User_ClaTask.h` alpha。
- PRESCALE=6是/4，不是/4.5；200MHz下ADCCLK为50MHz。ACQPS窗口按SYSCLK计，30/60/120对应155/305/605ns，不是2.7µs。PV周期5000差一个计数；AC alpha0.06在20kHz为约197Hz。
- 影响：跨ADC时序证明、滤波延时、PLL相位和I2C等时间假设不能沿用注释。电压/电流不同滤波也会影响功率测量相位。
- 修复：Broad集中管理实际时钟、频率与物理单位，派生寄存器值并校验；区分控制原始量与遥测滤波量。采样窗是否够模拟前端建立时间仍需实测。

### F27 · P1 · 异步参数保存完成时清掉新产生的dirty【确定可达交错】

- 位置：`Platform/Common/src/EepromParam.c:495`、`:613`、`:686`。
- 写入进行中Modbus可更新参数并MarkDirty；本次HEADER写完无条件ClearDirty。新值若未进入本次快照，会丢掉后续保存请求。
- 修复：分离本次快照和下一批dirty，使用generation或双脏集合；完成仅确认本次版本，保留写期间的新变化。
- 验收：在DATA和HEADER每个步骤分别修改同一/不同参数，断电重读后必须为已确认保存的最新版本。

### F28 · P2 · 参数持久化只有单份，掉电中途无法保留上一有效版本【确定设计限制】

- 位置：`EepromParam.c:302`、`:613`。
- 原地改数据再写CRC头，断电可能使唯一一份数据CRC失效；CRC检测损坏不能恢复旧值。版本仅描述格式时，点表排序/含义变化也需迁移策略。
- 修复：A/B记录或日志，序号+长度+schema ID+CRC+最后提交标记；至少保留上一有效记录。
- 验收：逐片断电故障注入，只允许读到完整旧版或完整新版；不接受混合版。

### F29 · P2 · 软件 I2C 时序依赖优化级，缺少SCL实际电平等待【确定实现限制，速率待测】

- 位置：`Drivers/src/Hw_I2cDriver.c`延时与位传输；Broad stI2cCfg注释。
- 注释称循环系数按-O2标定，本次Debug编译未指定该优化级；空循环不等于稳定微秒。释放SCL后未验证实际拉高，不能处理拉伸/总线受阻。
- 修复：基于可靠计时或硬件I2C，明确支持的器件时序；SCL高等待必须有超时和总线恢复。只面向不拉伸FRAM时也应记录限制。
- 验收：测SCL高低、SDA建立/保持、ACK/NACK、总线卡低；上层超时不允许阻塞控制。

### F30 · P2 · eCAP 首次值和旧值缺少有效期；XINT事件快照非原子【确定】

- 位置：`Drivers/src/Hw_EcapDriver.c`捕获读取/频率换算；`Drivers/src/Hw_ExitDriver.c` Hw_ExitTakeCount、Hw_ExitClearAll。
- eCAP第一次CAP1是启动到首边沿间隔，不是完整周期；失去输入后旧CAP仍可被当作有效频率。XINT取差值后再次读取累加器更新last，中间进ISR的事件可能被吞掉；ClearAll无条件EINT破坏调用者原先的关中断状态。
- 修复：捕获序号、首次无效、时间戳与超时；计数器只取一次本地快照；临界区保存/恢复原INTM。
- 验收：任意相位启动、断开过零输入、计数回绕、读取中间插入事件；关中断上下文调用后状态不变。

### F31 · P2 · 底层配置失败静默返回，数组/输入边界契约不足【确定接口问题】

- 位置：`Drivers/src/Hw_ClaDriver.c` MSGxINIT超时；`Hw_InvDriver.c:69`；各驱动Init返回void。
- 关键初始化失败不能上报到统一Ready；INV按表phaseNum写固定3项状态数组，接口未限制>3；duty浮点比较不能拒绝NaN。当前表是3相因此越界并未在当前配置触发，但通用API不能保证有效输入。
- 修复：配置先整体验证，再执行；返回模块/项目/寄存器错误；所有有限浮点、数量、ID和冲突进行校验；失败保留功率禁止，禁止部分配置后继续启动。
- 验收：非法ID/数量/空表/NaN/超时配置注入，不越界、不发波、可诊断。

### F32 · P2 · 启动入口和构建参数缺少可复现实物验证【确定缺口】

- 位置：`CMD/2837xS_FLASH_APP_CLA_lnk.cmd`，`.cproject`，`Debug/makefile`。
- APP入口0x86000依赖0x80000的Bootloader跳转，当前工程不能证明上电Flash启动路径；CCS直接加载并运行不覆盖这一点。工程元数据22.6.2与实际make22.6.4不一致，driverlib源和多套COFF/EABI库并列。容量注释混用C28x字和字节，地址长度应按16位word解释。
- 修复：记录实际编译器/ABI/优化，收敛唯一driverlib方式，明确Bootloader契约或独立启动配置；在单元表中统一words与bytes。
- 验收：移除调试器干预冷启动、软件复位、看门狗复位均正确；保存map、配置和板载版本证据。

### F33 · P2 · 最坏执行时间和共享数据一致性未得到证明【待实测/结构风险】

- 位置：`CtrlIsr.c`、`AdcSample.c`、`Schedule.c`及ISR/后台共享状态。
- 20kHz时预算50µs；ISR含三角函数、分频状态机、RMS收口等，平均执行时间不能替代最坏值；非嵌套ISR会延后SCI与Timer。多字段测量和状态缺少完整快照协议，单个volatile也不能保证跨字段一致。
- 修复：记录最坏延时、入ISR间隔和deadline miss；采样发布双缓冲/序列一致性快照，审计跨上下文变量；将非实时任务移出快环。
- 验收：RMS收口+通信满载+参数写入+全部保护路径同时发生时无漏拍；栈水位有测量裕量。当前没有证明超时，也没有证明满足预算。

### F34 · P1 · Modbus 字序条件用enum参与预处理，实际低字在前【确定】

- 位置：`User_App/Comm/inc/ModbusSlave.h:75`、`:79`；`User_App/Comm/src/ModbusSlave.c:145`及所有`#if (MB_WORD_HIGH_FIRST == 0)`。
- MB_WORD_HIGH_FIRST是枚举常量，不是宏；预处理阶段把未知标识符当0，故始终选择低字在前。声明的MB_WORD_ORDER_SEL也没有被这些条件使用。按“高字在前”解释浮点/U32的对端会得到错误值。
- 修复：使用明确宏常量比较配置选择，或在C表达式中选择；统一协议字序并加入固定32位向量。
- 验收：0x12345678以及float 1.0在03读取/10写入时高低字位置符合协议；不能只用全0测试。

### F35 · P2 · Modbus 宽变量逐半字读写，缺少整值校验/原子发布【确定接口缺陷】

- 位置：`ModbusSlave.c:117`、`:196`及MbWriteMulti逐字循环；PC点32为可写保存float。
- 每次写16位就与旧半字拼接、限幅并立即发布，临时值可能先被限幅改变另一半；NaN的大小比较均不成立，能绕过float范围检查。读取两个字也分别从实时变量取值，ISR更新时可能拼成非同一时刻的数据。U32写入复用int32限幅对大于INT32_MAX的通用值不正确；当前可写demo以float为主，影响范围需与实际点表一起判断。
- 修复：先解析完整请求到暂存区，对每个业务变量拼成整值、检查finite/类型范围，再一次提交；多字变量禁止孤立06半写或提供明确定义。读请求获取一致快照。
- 验收：浮点边界、NaN/Inf、跨符号、0x7FFFFFFF/0x80000000和ISR并发更新；确认协议写成功对应最终整值。

## 五、已排除的疑点，避免错误修复

1. **CAN_IF1MCTL.TXRQST=0不代表此次发送无请求。** 当前IF1CMD.TXRQST=1会覆盖MCTL中的该位，TRM p.2466明确规定。不要把这一点当作CAN发送根因。
2. **SCI FIFO接收不是必须同时开RXBKINTENA。** TRM Table18-4 p.2176区分FIFO与非FIFO中断选择；当前主要问题是读FIFO、尾包和错误恢复。
3. **ADC INT连续模式不要求每次先清INT旗标才有下一脉冲。** 当前INT1CONT/INT2CONT=1有效；仍需管理溢出和确认触发频率。
4. **CLA Task/辅助函数已编译和链接。** MAP中有Task1/2、Cla_Lpf、scratchpad，不能仅凭函数少一个pragma判定缺失。
5. **GPIO映射不作为本次重新布线建议。** 但已接GPIO后的X-BAR选择错误是独立问题。
6. **Hw_EnablePwmClocks把EPWM9–12误写PCLKCR3确有问题，但不是当前缺波的唯一解释。** 此辅助函数未参与当前调用路径，InitSysCtrl已开PWM时钟；应修正为PCLKCR2正确位，但标为潜在缺陷。
7. **硬件TZ故障一般不会自动停止PWM时基/SOC。** 清TZ是恢复输出的受控操作，不应为“让CLA中断进来”而无条件执行。

## 修复顺序

先F01/F02/F03/F10/F11/F18保障禁止与故障路径；再F04/F05/F06/F07验证采样链；随后F08/F09/F12验证调制原语；并行修复通信F13–F17。用最小验证应用替换自动状态机后完成底层验收，再讨论F19及控制器设计。配置/实现整改与实际台架验收必须分别记录，不能把本文建议直接标成已修复。

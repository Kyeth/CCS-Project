# F28374S 最小外设框架

本分支从 `f7978e1` 建立，保留 `Broad` 配置层、`Drivers` 驱动层、TI 器件支持和链接脚本。参考 `deepseek/pcs-stepwise` 分支下 `reference/IB800U` 的生命周期入口风格，`main()` 依次调用 `PeripheralsInit`、`PlatformInit`、`ApplicationInit`、`TaskCreate`、`StartSystem`，循环中调用 `Hw_FeedWatchDog` 和 `Task_Run`。当前 `Platform/Schedule` 只提供 1 ms 调度节拍（由 CPU Timer1 的 100 µs 硬件中断 10 分频得到）的协作式调度骨架；应用对象和后台任务尚未创建，也没有控制应用功能。原有 `docs/review/2026-10-29` 是旧版审阅档案，不能作为本分支的当前运行说明。

## 配置来源与时序

管脚依据为项目所有者提供的 `15KW软件管脚整理文档.xlsx` Sheet1，SHA-256 `9271e429ac2a8cd0131a9e1ee1d057cc498226897584d7298689ed91d7c77826`。工作簿未进入本分支，复核时传入其本机路径。Broad 表中的硬件资源配置集中在 `Broad/src/DrvCfg_EP15K.c`。

| 项目 | 配置 |
| --- | --- |
| 晶振 / SYSCLK / ePWMCLK / LSPCLK | 10 / 200 / 100 / 50 MHz |
| INV、BUS | ePWM 上下计数，TBPRD=2500，20 kHz |
| PV1–PV4 | ePWM 向上计数，TBPRD=4999，20 kHz；共同启动时预置 TBCTR=0/1250/2500/3750，对应初始相位 0/90/180/270°；运行中独立计数 |
| 外置风扇 | EPWM12B，上下计数，TBPRD=5000，10 kHz；动作写在 B 通道 |
| ADC | ADCA/B/C/D，50 MHz ADC 时钟；EPWM1 SOCA 在 CTR=ZERO 触发 SOC；ADCA SOC15 EOC 产生 ADCINT1 |
| CLA / CPU | ADCINT1 → CLA Task1（只查表发布下一拍 MUX 地址，不读 ADC）→ PIE11.1 主控制 ISR；CPU 在 ISR 里读全部 20 路结果寄存器、按地址归档 MUX、跑零漂状态机，并更新 MUX 地址 GPIO46/47/42 供下一采样周期使用 |
| 心跳 | CPU Timer1，硬件中断 100 µs（PRD=19999），软件 10 分频得 1 ms；CPU INT13；记录 `u32Tick100us`、1 ms 心跳与主中断停滞计数 |
| 后台任务 | 定时器 ISR 只登记 1 ms 节拍；`Task_Run` 在主循环消费节拍，当前注册 1 个任务：1 kHz 显示层滤波 `AdcSampleTask`。通信前台不再按 1 ms 注册：100 µs 心跳置通知位，主循环 `PortTaskService()` 每拍领取一次、只服务一个单元 |
| SCI | A 19200、C 115200、D 38400，8N1；收发采用轮询 API。步骤 05C 起：接收由 100 µs 心跳每拍服务一路（A→C→D 轮转，每路标称 300 µs，每次最多 8 次读）；发送在前台五单元轮转里（SCI-A/C/D TX 与 CAN-A/B），每 100 µs 一次服务机会、每路标称 500 µs、单元预算 100 µs |
| CAN | A 500 kbit/s、B 1 Mbit/s，外部总线模式；逐接收对象过滤（A 只收标准数据帧、B 只收扩展数据帧，ID/掩码暂为 0 作过渡）；运行期 API 带时间预算，超时用 Timer0 时间基准；当前不发送应用报文 |

MUX 轮询地址为 `0→1→2→3→4→6→7→0`，跳过未定义的地址 5。B3（MUX2）在地址 0~3 输出 PV1~PV4 电压、地址 4 输出 VISO；共用同一组地址线的 D3（MUX1）只在地址 6、7 输出 BUSL（下半母线）和 BUSH（上半母线）。

采样分五层放在 `Platform/Sample/`：L1 `stSampleRaw`（原始码值，20 kHz）、L2 `stSampleIns`（减零漂、乘系数后的工程值，20 kHz）、L3 `stSampleFlt`（块平均：Avg8 8 拍 400 µs、AvgHalf 半个工频周期、AvgMux 16 轮 ≈ 5.6 ms）、L4 `stSampleAc`（每工频周期的 RMS 与峰值，窗口由 `stPLLData.ThetaR` 回绕收口）、L5 `stSampleDisplay`（1 kHz 一级 IIR，α=0.005，专供显示）。18 路固定通道在上电 3 s 后跟踪 200 ms 的平均值作为零漂，超差 ±15% 判失败并保留标称零点；4 路 MUX 通道单极性无偏置，不参与零漂。

额定频率默认 50 Hz，可用 `SampleSetFreqMode()` 在 50 / 60 Hz 之间切换，或用 `SampleSetNominalFreq()` 写入 45~65 Hz 的任意值；切换只刷新 `stSampleCfg.fNominalFreq / u16HalfSteps`（50 Hz 200 拍、60 Hz 167 拍）与交流窗口上限，不打断正在累加的块。角度来源是 `Platform/Pll/Inc/PllData.h` 的 `stPLLData`（`ThetaR` / `ThetaS` / `ThetaT`，唯一写入方是尚未落地的 PLL 模块），当前全零，L4 因此靠样本上限强制收口并置 `SAMPLE_AC_VALID_FORCED`。控制环取 L2 瞬时值或 Avg8；AvgHalf 用于有功与直流分量；MUX 通道刷新率 5.6 ms，只用于监测、保护与显示。

PV 的 EPWM5/6 位于由 EPWM4 控制同步输入的链内，而 EPWM4 用于 INV。PV 四路因此设置 `PHSEN=0`、`SYNCOSEL=DISABLE`，在 `TBCLKSYNC=0` 时预置各自 `TBCTR`，由同一次 `TBCLKSYNC=1` 开始运行。驱动先以立即模式写入活动 `TBPRD`，再写入相同的影子值，避免非零启动计数跳过首次零点装载。此方案仅提供启动时的错相，不提供运行中的周期重新对齐；改变 PV 周期或时钟分频后必须重新评估错相。依据为项目所有者提供的 F2837xS 技术参考手册 SPRUHX5I 第 14.4.3.3、14.4.4 节。

## 安全默认态

GPIO88 在 GPIO 表首项配置为低，板上驱动使能保持关闭；继电器、风扇开关和预充等控制 GPIO 默认低。所有 12 个 ePWM 模块执行 `TZFRC.OST`，并保持锁存。时基和 ADC/CLA 触发仍可运行。此镜像不提供清 OST 或升高 GPIO88 的接口。

PV1/2/3/4 过流 GPIO13/17/41/40、INV 过流/短路 GPIO81/82 经 INPUT X-BAR 与 ePWM X-BAR 接到各自 TRIP；外部正常高、故障低，独立 TRIP 反相后进入 DCAH/DCBH 的 CBC。BUS 故障 GPIO77/80 已作为异步输入配置，但六路可直连 ePWM X-BAR 的 INPUT1–6 已分配给 PV 和 INV，EPWM8 **没有专属硬件 TZ 故障源**。启用 BUS 功率输出前须另行确定硬件保护路由并实测。INV/PV 的 CBC 极性、门极有效电平及 80 TBCLK 死区也须在断开功率级的台架上核验。

05C 前台轮转已登记为**可用基线**（2026-11-09）：三路自环 60.6 s 零错误、吞吐达标、停发排空后收发帧数与字节数完全一致、单路故障不影响另两路、控制不停滞；控制入口 62.2／37.8 µs 抖动单列跟踪，`S-05B-08/09` 保留历史未达标记录。CAN 保持不动，SCI 不再追加性能微调。

观察量统一放在 `stBringupStatus` 结构体中：`bReady=1` 表示驱动初始化调用通过并已启用 ePWM 时基，不代表实板外设链路验收；`unError` 的位 0/1/2/3 分别表示 CLA 消息 RAM、SCI、CAN（含 Timer0 时间基准未能启动）、时钟配置检查失败；位 4 `bPortInitError` 表示 CAN-B 帧类型绑定失败，位 5 `bTaskCreateError` 表示 `AdcSampleTask` 或 `PortTask` 未注册成功。即使初始化通过，所有功率输出也仍封锁。`u16CalState`（0 空闲/1 跳过/2 跟踪/3 就绪/4 失败）与 `bCalReady` 反映采样层零漂状态：只有 `bCalReady=1` 时 `stSampleIns` 才有意义。

## 构建与静态复核

在本分支根目录，使用 TI CGT C2000 22.6.2.LTS：

```powershell
python tools/build_minimal.py --cgt 'D:\ti\CCS1110\ccs\tools\compiler\ti-cgt-c2000_22.6.2.LTS'   # 按本机实际安装位置改；脚本默认值是 E:\CCS\...
python tools/verify_config.py --pin-sheet 'Z:\Git1029\CCS-Project\docs\review\2026-10-29\15KW软件管脚整理文档.xlsx'
```

独立构建产物默认在项目目录外的 `CCS-Project-minimal-build/`，避免 CCS 工程扫描到另一套 `.obj`。步骤 01 加入采样层后，用 CGT 22.6.2.LTS 完成 29 个编译单元、零编译警告、链接成功；步骤 02 加入 L3/L4/L5 与 `Platform/Pll` 后为 30 个编译单元，同样零警告、链接成功；步骤 03 把硬件心跳改为 100 µs 后仍为 30 个编译单元、零警告，`EP15K_minimal.out` 671,632 字节；步骤 04 加入 CAN 驱动修正与 CPU Timer0 时间基准后为 31 个编译单元；步骤 05A 加入端口层与环形缓存后为 34 个；步骤 05B 加入 `PortCritical.c` 与 SCI 恢复状态机后为 35 个编译单元、零警告，`EP15K_minimal.out` 775,148 字节（sha256 `78112ce2…6eb4c8d`）；步骤 05C 加入前台五单元轮转后为 37 个编译单元、零警告，生产态 `EP15K_minimal.out` 786,740 字节（sha256 `4309649d…18207`，`.text 0x45AF`、`RAMGS0` 余 `0x14E`），测量态 804,964 字节（sha256 `0709f765…1a9`）IB800U 风格改写后曾由 CCS headless 使用 CGT 22.6.4.LTS 清理后全量构建，生成 `.out/.hex`，0 错误、0 警告。当前 PV 独立运行配置的静态核对为 132 项通过。所有构建产物只供本机复核，未做烧录或实板测试。

## 首次实板验收顺序

1. 保持功率级隔离，先量 GPIO88、继电器输出以及所有门极输出的安全电平；记录复位到 `main` 期间的电平。
2. 用调试器观察 `stBringupStatus.unError.all=0`、`stBringupStatus.bReady=1`，寄存器确认 SYSCLK/ePWMCLK、各 TBPRD、OST 锁存和 ADC SOC/CLA 向量。
3. 计时 `stBringupStatus.u32ControlIsrCount` 约每秒增加 20,000、`stBringupStatus.u32Tick100us` 约每秒增加 10,000、`stBringupStatus.u32HeartbeatMs` 约每秒增加 1,000、`stBringupStatus.u16ControlStallMs` 保持 0；核对 CLA 序号、MUX 地址序列、`stSampleRaw.u32Seq` 与 `stBringupStatus.u32ControlIsrCount` 同步增长、ADCINT 溢出标志。
4. 用递增加载字节流核对三路 SCI 的接收发布与 `SciPortRxUsed/MaxGap/RxErrEvents`，并复核 `.map` 中 `RAMGS1_PORTBUF` 0x0400 / `.portbuf` 0x180；4'. 用外部已知低压信号逐个核对 ADC 通道和 MUX 建立时间；分别核对三路 SCI 及两路 CAN 的收发引脚、速率和收发器状态；另需核对 CAN 逐对象过滤（`UMask/MXtd/MDir`）与 `NDAT_21`/`TXRQ_21` 状态查询是否与实测一致。
5. 单独向每个故障 GPIO 注入低电平，读回 INPUT X-BAR、TRIP/CBC 标志和输出锁存。BUS 硬件 TZ 路由与门极极性确认前不得进入输出放行测试。

芯片同步链依据 [TI F2837xS 数据手册 6.11.2.1](https://www.ti.com/lit/ds/symlink/tms320f28379s.pdf)；本项目 GPIO 复用码同时与仓库内 TI `pin_map.h` 核对。

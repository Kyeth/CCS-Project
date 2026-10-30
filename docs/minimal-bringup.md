# F28374S 最小外设框架

本分支从 `f7978e1` 建立，保留 `Broad` 配置层、`Drivers` 驱动层、TI 器件支持和链接脚本。参考 `deepseek/pcs-stepwise` 分支下 `reference/IB800U` 的生命周期入口风格，`main()` 依次调用 `PeripheralsInit`、`PlatformInit`、`ApplicationInit`、`TaskCreate`、`StartSystem`，循环中调用 `Hw_FeedWatchDog` 和 `Task_Run`。当前 `Platform/Schedule` 只提供 1 ms 心跳驱动的协作式调度骨架；应用对象和后台任务尚未创建，也没有控制应用功能。原有 `docs/review/2026-10-29` 是旧版审阅档案，不能作为本分支的当前运行说明。

## 配置来源与时序

管脚依据为项目所有者提供的 `15KW软件管脚整理文档.xlsx` Sheet1，SHA-256 `9271e429ac2a8cd0131a9e1ee1d057cc498226897584d7298689ed91d7c77826`。工作簿未进入本分支，复核时传入其本机路径。Broad 表中的硬件资源配置集中在 `Broad/src/DrvCfg_EP15K.c`。

| 项目 | 配置 |
| --- | --- |
| 晶振 / SYSCLK / ePWMCLK / LSPCLK | 10 / 200 / 100 / 50 MHz |
| INV、BUS | ePWM 上下计数，TBPRD=2500，20 kHz |
| PV1–PV4 | ePWM 向上计数，TBPRD=4999，20 kHz；共同启动时预置 TBCTR=0/1250/2500/3750，对应初始相位 0/90/180/270°；运行中独立计数 |
| 外置风扇 | EPWM12B，上下计数，TBPRD=5000，10 kHz；动作写在 B 通道 |
| ADC | ADCA/B/C/D，50 MHz ADC 时钟；EPWM1 SOCA 在 CTR=ZERO 触发 SOC；ADCA SOC15 EOC 产生 ADCINT1 |
| CLA / CPU | ADCINT1 → CLA Task1（按地址分别滤波 MUX B3/D3）→ PIE11.1 主控制 ISR；CPU 更新 MUX 地址 GPIO46/47/42，供下一采样周期使用 |
| 心跳 | CPU Timer1，1 ms，CPU INT13；仅记录时基与主中断停滞计数 |
| 后台任务 | 定时器 ISR 只登记 1 ms 节拍；`Task_Run` 在主循环消费节拍，当前注册任务数为 0 |
| SCI | A 19200、C 115200、D 38400，8N1；收发采用轮询 API |
| CAN | A 500 kbit/s，B 1 Mbit/s；外部总线模式，当前不发送应用报文 |

MUX 轮询地址为 `0→1→2→3→4→6→7→0`，跳过未定义的地址 5。地址 4 是 VISO；当前仅发布带地址标签的滤波 ADC 码值，不做电压换算或保护判定。

PV 的 EPWM5/6 位于由 EPWM4 控制同步输入的链内，而 EPWM4 用于 INV。PV 四路因此设置 `PHSEN=0`、`SYNCOSEL=DISABLE`，在 `TBCLKSYNC=0` 时预置各自 `TBCTR`，由同一次 `TBCLKSYNC=1` 开始运行。驱动先以立即模式写入活动 `TBPRD`，再写入相同的影子值，避免非零启动计数跳过首次零点装载。此方案仅提供启动时的错相，不提供运行中的周期重新对齐；改变 PV 周期或时钟分频后必须重新评估错相。依据为项目所有者提供的 F2837xS 技术参考手册 SPRUHX5I 第 14.4.3.3、14.4.4 节。

## 安全默认态

GPIO88 在 GPIO 表首项配置为低，板上驱动使能保持关闭；继电器、风扇开关和预充等控制 GPIO 默认低。所有 12 个 ePWM 模块执行 `TZFRC.OST`，并保持锁存。时基和 ADC/CLA 触发仍可运行。此镜像不提供清 OST 或升高 GPIO88 的接口。

PV1/2/3/4 过流 GPIO13/17/41/40、INV 过流/短路 GPIO81/82 经 INPUT X-BAR 与 ePWM X-BAR 接到各自 TRIP；外部正常高、故障低，独立 TRIP 反相后进入 DCAH/DCBH 的 CBC。BUS 故障 GPIO77/80 已作为异步输入配置，但六路可直连 ePWM X-BAR 的 INPUT1–6 已分配给 PV 和 INV，EPWM8 **没有专属硬件 TZ 故障源**。启用 BUS 功率输出前须另行确定硬件保护路由并实测。INV/PV 的 CBC 极性、门极有效电平及 80 TBCLK 死区也须在断开功率级的台架上核验。

`gBringupReady=1` 表示驱动初始化调用通过并已启用 ePWM 时基，不代表实板外设链路验收。`gBringupError` 位 0/1/2/3 分别表示 CLA 消息 RAM、SCI、CAN、时钟配置检查失败。即使初始化通过，所有功率输出也仍封锁。

## 构建与静态复核

在本分支根目录，使用 TI CGT C2000 22.6.2.LTS：

```powershell
python tools/build_minimal.py --cgt 'E:\CCS\ccs\tools\compiler\ti-cgt-c2000_22.6.2.LTS'
python tools/verify_config.py --pin-sheet 'Z:\Git1029\CCS-Project\docs\review\2026-10-29\15KW软件管脚整理文档.xlsx'
```

独立构建产物默认在项目目录外的 `CCS-Project-minimal-build/`，避免 CCS 工程扫描到另一套 `.obj`。当前版本用 CGT 22.6.2.LTS 完成 24 个编译单元、零编译警告、链接成功；IB800U 风格改写后曾由 CCS headless 使用 CGT 22.6.4.LTS 清理后全量构建，生成 `.out/.hex`，0 错误、0 警告。当前 PV 独立运行配置的静态核对为 132 项通过。所有构建产物只供本机复核，未做烧录或实板测试。

## 首次实板验收顺序

1. 保持功率级隔离，先量 GPIO88、继电器输出以及所有门极输出的安全电平；记录复位到 `main` 期间的电平。
2. 用调试器观察 `gBringupError=0`、`gBringupReady=1`，寄存器确认 SYSCLK/ePWMCLK、各 TBPRD、OST 锁存和 ADC SOC/CLA 向量。
3. 计时 `gControlIsrCount` 约每秒增加 20,000、`gHeartbeatMs` 约每秒增加 1,000，`gControlStallMs` 保持 0；核对 CLA 序号与 MUX 地址序列、ADCINT 溢出标志。
4. 用外部已知低压信号逐个核对 ADC 通道和 MUX 建立时间；分别核对三路 SCI 及两路 CAN 的收发引脚、速率和收发器状态。
5. 单独向每个故障 GPIO 注入低电平，读回 INPUT X-BAR、TRIP/CBC 标志和输出锁存。BUS 硬件 TZ 路由与门极极性确认前不得进入输出放行测试。

芯片同步链依据 [TI F2837xS 数据手册 6.11.2.1](https://www.ti.com/lit/ds/symlink/tms320f28379s.pdf)；本项目 GPIO 复用码同时与仓库内 TI `pin_map.h` 核对。

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""A03 静态配置重放：把 Broad 配置表 + 驱动写入逻辑与 expected_config.json 比对。

设计原则（对应步骤 01 的审阅条件）：
- 期望值只来自 expected_config.json，且每项带 basis（TI_MANUAL/USER_CONFIRMED/
  RULED_DESIGN/PENDING）与 source；本脚本不从被检查表反向抄期望值。
- 本脚本不改固件、不访问硬件；结果是"静态核对"，不是仿真，也不是板上证据。
- 结论分四类：PASS（与期望一致）/ FAIL（与期望不一致）/ PENDING（期望本身待裁定）/
  INFO（只登记事实，不参与判定）。

用法：
    python config_replay.py --src <git archive 解包树> [--rev <提交SHA>] [--out <输出目录>]

`--src` 必须是**固定提交**的解包树（由 build_clean.py 的 `--rev` 产生），脚本不读
工作区；`--rev` 只用于在报告 meta 里记录该树对应的提交，并附每个源文件的 SHA-256
以便复核。
"""

import argparse
import datetime
import hashlib
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE_HASHES = {}

BROAD = "Broad/src/DrvCfg_EP15K.c"
ADC_DRV = "Drivers/src/Hw_AdcDriver.c"
XBAR_DRV = "Drivers/src/Hw_XbarDriver.c"
PWM_DRV = "Drivers/src/Hw_PwmDriver.c"
ADC_HDR = "device/headers/include/F2837xS_adc.h"

# ST_PWM_CFG_ITEM 字段顺序（Drivers/inc/Hw_PwmDriver.h:125-152）
PWM_FIELDS = [
    "u16PwmId", "u16PwmType", "u16Period", "u16CmpA", "u16CmpB", "u16Phase",
    "u16CtrMode", "u16SyncMode", "u16SyncCntLoadDir", "u16SyncInSel",
    "u16SyncOutSel", "bGload", "u16LinkPwm", "u16DeadTime", "u16TzActionA",
    "u16TzActionB", "u16InitForceA", "u16InitForceB", "stAqA", "stAqB", "stEtSoca",
]
TZ_FIELDS = ["u16PwmId", "u16DcaTrip", "u16DcbTrip", "u16DcaEvent",
             "u16DcbEvent", "u16DcaAction", "u16DcbAction"]
ADC_MOD_FIELDS = ["u16AdcId", "pstAdcChCfgItem", "u16Resolution", "u16SignalMode",
                  "bAdcIntEnable", "u16IntSel", "bAdcInt2Enable", "u16Int2Sel",
                  "u16SocStride"]


def write_text_lf(path, text):
    """统一写 UTF-8 + LF，避免 Windows 下 CRLF 让差异和哈希在平台间漂移。"""
    Path(path).write_bytes(text.encode("utf-8"))


def read_text(src, rel):
    path = Path(src) / rel
    data = path.read_bytes()
    SOURCE_HASHES[rel] = hashlib.sha256(data).hexdigest()
    return data.decode("utf-8", errors="replace")


def strip_comments(src):
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


def parse_item(s, i):
    """从 s[i]=='{' 开始解析一个初始化项，返回 (字段列表, 结束位置)。"""
    assert s[i] == "{"
    i += 1
    fields, cur = [], ""
    while i < len(s):
        c = s[i]
        if c == "{":
            if cur.strip():
                fields.append(cur.strip())
                cur = ""
            nested, i = parse_item(s, i)
            fields.append(nested)
            continue
        if c == "}":
            if cur.strip():
                fields.append(cur.strip())
            return fields, i + 1
        if c == ",":
            if cur.strip():
                fields.append(cur.strip())
            cur = ""
            i += 1
            continue
        cur += c
        i += 1
    raise ValueError("unbalanced initializer")


def parse_body(s):
    items, i = [], 0
    while i < len(s):
        if s[i] == "{":
            item, i = parse_item(s, i)
            items.append(item)
        else:
            i += 1
    return items


def array_items(text, name):
    """定位 `name[] = { ... }` 并解析成项列表。"""
    m = re.search(r"\b" + re.escape(name) + r"\s*\[\s*\]\s*=\s*\{", text)
    if not m:
        raise KeyError("array not found: %s" % name)
    start = text.index("{", m.end() - 1)
    depth, i = 0, start
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return parse_body(text[start + 1:i])
        i += 1
    raise ValueError("unbalanced array: %s" % name)


def tok_int(tok):
    if isinstance(tok, list):
        raise TypeError("expect scalar, got group")
    t = tok.strip().rstrip("uU")
    t = re.sub(r"^\((.*)\)$", r"\1", t)
    return int(t, 0)


def tok_str(tok):
    if isinstance(tok, list):
        raise TypeError("expect scalar, got group")
    return re.sub(r"^\((.*)\)$", r"\1", tok.strip())


def offset_of_ptr(tok):
    m = re.search(r"\[(\d+)\]", tok)
    return int(m.group(1)) if m else None


class Checker:
    def __init__(self, expected):
        self.exp = expected
        self.checks = []

    def add(self, cid, domain, title, basis, expected, actual, verdict, detail=""):
        self.checks.append({
            "id": cid, "domain": domain, "title": title, "basis": basis,
            "expected": expected, "actual": actual, "verdict": verdict,
            "detail": detail,
        })

    def cmp(self, cid, domain, title, exp_value, actual_value, detail="", basis=None):
        verdict = "PASS" if actual_value == exp_value else "FAIL"
        self.add(cid, domain, title, basis or "RULED_DESIGN", exp_value, actual_value,
                 verdict, detail)
        return verdict == "PASS"

    def cmp_design(self, cid, domain, title, exp_item, actual_value, detail=""):
        """期望值本身标 PENDING 时，值一致也只记 PENDING：配置一致 ≠ 设计已批准。"""
        exp_value, basis = exp_item["value"], exp_item.get("basis", "RULED_DESIGN")
        if actual_value != exp_value:
            verdict = "FAIL"
        elif basis == "PENDING":
            verdict = "PENDING"
        else:
            verdict = "PASS"
        self.add(cid, domain, title, basis, exp_value, actual_value, verdict, detail)
        return verdict

    def ex(self, path):
        node = self.exp
        for key in path.split("."):
            node = node[key]
        return node


def main():
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", required=True,
                    help="固定提交的 git archive 解包树（不读工作区）")
    ap.add_argument("--rev", default="", help="该树对应的提交 SHA（只用于记录）")
    ap.add_argument("--out", default=str(HERE))
    args = ap.parse_args()
    src = Path(args.src).resolve()
    outdir = Path(args.out).resolve()
    outdir.mkdir(parents=True, exist_ok=True)
    if not (src / BROAD).is_file():
        raise SystemExit("--src 中找不到 %s: %s" % (BROAD, src))

    exp = json.loads((HERE / "expected_config.json").read_text(encoding="utf-8"))
    ck = Checker(exp)

    broad = strip_comments(read_text(src, BROAD))
    broad_raw = read_text(src, BROAD)
    adc_drv_raw = read_text(src, ADC_DRV)
    pwm_drv_raw = read_text(src, PWM_DRV)
    xbar_drv_raw = read_text(src, XBAR_DRV)
    adc_hdr = read_text(src, ADC_HDR)

    # ---------------- 解析实际值 ----------------
    adc_ch = [dict(zip(["trig", "acqps", "ch"], [tok_int(t) for t in it]))
              for it in array_items(broad, "stAdcChCfgItem")]
    adc_mod = []
    for it in array_items(broad, "stAdcCfgItem"):
        d = dict(zip(ADC_MOD_FIELDS, it))
        d["base_offset"] = offset_of_ptr(d["pstAdcChCfgItem"])
        d["u16SocStride"] = tok_int(d["u16SocStride"])
        adc_mod.append(d)

    pwm = []
    for it in array_items(broad, "stPwmCfgItem"):
        d = dict(zip(PWM_FIELDS, it))
        for k in ("u16Period", "u16CmpA", "u16CmpB", "u16DeadTime", "u16Phase", "bGload"):
            d[k] = tok_int(d[k])
        pwm.append(d)
    pwm_by_id = {d["u16PwmId"]: d for d in pwm}

    tz = [dict(zip(TZ_FIELDS, [tok_str(t) for t in it]))
          for it in array_items(broad, "stPwmTzCfgItem")]

    xbar_in = [(tok_str(i[0]), tok_int(i[1])) for i in array_items(broad, "stXbarInputCfgItem")]
    xbar_mux = [(tok_str(i[0]), tok_int(i[1]), tok_str(i[2]))
                for i in array_items(broad, "stXbarTripMuxCfgItem")]
    xbar_out = [(tok_str(i[0]), tok_int(i[1])) for i in array_items(broad, "stXbarTripOutCfgItem")]

    # 驱动常量
    def drv_const(name, src):
        m = re.search(r"#define\s+" + name + r"\s+\(?(\d+)[uU]?\)?", src)
        return tok_int(m.group(1)) if m else None

    shifts = {k: drv_const("HW_ADC_SOCCTL_%s_SHIFT" % k, adc_drv_raw)
              for k in ("ACQPS", "CHSEL", "TRIGSEL")}
    prescale = drv_const("HW_ADC_CLK_PRESCALE_DIV_4", adc_drv_raw)
    m = re.search(r"ADCSOCPRICTL\.bit\.SOCPRIORITY\s*=\s*(\d+)U?", adc_drv_raw)
    soc_priority = int(m.group(1)) if m else None
    m = re.search(r"ADCCTL1\.bit\.INTPULSEPOS\s*=\s*(\d+)U?", adc_drv_raw)
    intpulsepos = int(m.group(1)) if m else None

    # 器件位域：ADCSOC0CTL_BITS 的字段起始位
    hdr_bits = {}
    body = adc_hdr[adc_hdr.index("struct ADCSOC0CTL_BITS"):]
    body = body[:body.index("};")]
    for m in re.finditer(r"Uint(?:16|32)\s+(\w+)\s*:\s*\d+\s*;\s*//\s*(\d+):(\d+)", body):
        hdr_bits[m.group(1)] = int(m.group(3))

    # ---------------- ADC 检查 ----------------
    dom = "adc"
    exp_adc = exp["adc"]
    strides = [d["u16SocStride"] for d in adc_mod]
    exp_stride = exp_adc["soc_stride"]["value"]
    ck.add("A03-ADC-01", dom, "四个 ADC 模块的 SOC 表步长 = %d" % exp_stride,
           exp_adc["soc_stride"]["basis"], exp_stride, strides,
           "PASS" if all(s == exp_stride for s in strides) else "FAIL",
           "Hw_AdcDriver.c 用 soc*stride 索引交织表；步长错误会导致跨模块取到别的通道")
    bases = [d["base_offset"] for d in adc_mod]
    ck.cmp("A03-ADC-02", dom, "模块基址偏移 = [0,1,2,3]",
           exp_adc["module_base_offsets"]["value"], bases)
    ck.cmp("A03-ADC-03", dom, "SOCCTL 位域移位与器件头文件一致",
           exp_adc["socctl_bit_lsb"]["value"], shifts,
           detail="驱动器 macro 的移位值 vs F2837xS_adc.h:324-331 的字段起始位 (header=%s)" % hdr_bits)

    soc15 = adc_ch[15 * 4 + 0]
    ck.cmp_design("A03-ADC-04", dom, "SOC15(A) 的触发源 5 / ACQPS 120 与配置表一致",
                  exp_adc["anchor_soc15"],
                  {"trig": soc15["trig"], "acqps": soc15["acqps"]},
                  detail="值一致只说明表内自洽；SOC15 是否真为本批最后完成、能否充当 ADCINT1 "
                         "屏障，需要跨 ADC 的每 SOC 起止时刻证明，本步骤没有该证明"
                         "（01_current_logic.md:95、03_validation_plan.md:71）")

    a = adc_mod[0]
    e05 = {"int1_sel": exp_adc["int1_sel"]["value"], "int2_sel": exp_adc["int2_sel"]["value"],
           "cont": exp_adc["int_cont"]["value"]}
    a05 = {"int1_sel": tok_int(a["u16IntSel"]), "int2_sel": tok_int(a["u16Int2Sel"]),
           "cont": 1 if (tok_int(a["bAdcIntEnable"]) and tok_int(a["bAdcInt2Enable"])) else 0}
    basis05 = "PENDING" if any(exp_adc[k].get("basis") == "PENDING"
                               for k in ("int1_sel", "int2_sel", "int_cont")) else "RULED_DESIGN"
    ck.add("A03-ADC-05", dom, "ADC-A 的 INT1/INT2 触发 SOC 与连续模式", basis05, e05, a05,
           "FAIL" if a05 != e05 else ("PENDING" if basis05 == "PENDING" else "PASS"),
           "INT1SEL=15 依赖 SOC15 屏障假设，INT2SEL=9 依赖 CLA Task2 读 SOC8/9 的配对；"
           "两者都只是与现有代码一致，缺少独立设计依据")

    ck.cmp("A03-ADC-06", dom, "ADCSOCPRICTL.SOCPRIORITY = 16（固定优先级）",
           exp_adc["soc_priority"]["value"], soc_priority)
    ck.cmp("A03-ADC-07", dom, "ADCCTL2.PRESCALE = 6（ADCCLK = SYSCLK/4）",
           exp_adc["adcclk_prescale"]["value"], prescale,
           detail="SYSCLK=200MHz 时 ADCCLK=50MHz，需 ≤ 器件上限；实际 SYSCLK 由时钟配置决定，属待板核对项")
    ck.cmp("A03-ADC-08", dom, "ADCCTL1.INTPULSEPOS = 1（EOC 时出中断脉冲）",
           exp_adc["intpulsepos"]["value"], intpulsepos)

    partial = [i for i, c in enumerate(adc_ch)
               if (c["trig"], c["acqps"], c["ch"]) != (0, 0, 0)
               and (c["trig"] == 0 or c["ch"] == 0 and c["acqps"] == 0)]
    ck.cmp("A03-ADC-09", dom, "未使用项全 0（驱动据此跳过写入）",
           exp_adc["unused_soc_word"]["value"], 0,
           detail="部分非零项索引: %s" % partial)
    if partial:
        ck.checks[-1]["verdict"] = "FAIL"
        ck.checks[-1]["actual"] = "部分非零项 %s" % partial

    ck.add("A03-ADC-10", dom, "ADC 通道→板上信号映射", "PENDING",
           "PENDING（需原理图核对）",
           "%d 个已配置 SOC" % sum(1 for c in adc_ch if (c["trig"], c["acqps"], c["ch"]) != (0, 0, 0)),
           "PENDING", "通道语义不在本步骤解码；表中的信号注释只作为现状记录")

    # ---------------- X-BAR 检查 ----------------
    dom = "xbar"
    exp_x = exp["xbar"]
    exp_in = exp_x["inputs"]["value"]
    act_in = {k: v for k, v in xbar_in}
    ck.cmp("A03-XB-01", dom, "InputX-BAR 引脚映射", exp_in, act_in,
           detail="硬件事实，依据为用户确认的 GPIO 分配")

    exp_mux = exp_x["trip_mux"]["value"]
    act_mux = {t: m for t, m, _ in xbar_mux}
    ck.cmp("A03-XB-02", dom, "TRIP↔MUX 映射（INPUT1..6 → MUX1/3/5/7/9/11）",
           exp_mux, act_mux,
           detail="TRM Table 8-2：奇数 MUX 组选择该组 INPUTXBAR 源")
    ok_mode = all(m == exp_x["trip_mux_mode"]["value"] for _, _, m in xbar_mux)
    ck.cmp("A03-XB-03", dom, "TRIP MUX 均为 ENABLE 模式（选择 INPUTXBAR 源）",
           True, ok_mode)

    inv_sep = exp_x["inv_trip_separation"]["value"]
    mux_of = {t: m for t, m, _ in xbar_mux}
    act_sep = {"HW_XBAR_TRIP9": "HW_XBAR_INPUT4", "HW_XBAR_TRIP10": "HW_XBAR_INPUT5"} \
        if mux_of.get("HW_XBAR_TRIP9") == 7 and mux_of.get("HW_XBAR_TRIP10") == 9 else \
        {"HW_XBAR_TRIP9": "MUX%s" % mux_of.get("HW_XBAR_TRIP9"),
         "HW_XBAR_TRIP10": "MUX%s" % mux_of.get("HW_XBAR_TRIP10")}
    ck.cmp("A03-XB-04", dom, "逆变过流与短路分路到 TRIP9/TRIP10", inv_sep, act_sep,
           detail="F02 修复意图：低有效故障不得先或后判断")

    invert = {t: v for t, v in xbar_out}
    ck.add("A03-XB-05", dom, "TRIP 输出极性 TRIPOUTINV", "PENDING",
           "PENDING（表值与注释冲突：表=1 全反相，注释=不取反）",
           invert, "PENDING",
           "极性取决于比较器输出与板上网络；需原理图/台架裁定，本步骤不下结论")

    stale_comment = "共用 TRIP9" in broad_raw
    ck.add("A03-XB-06", dom, "X-BAR 注释与表实现一致", "RULED_DESIGN",
           "注释应说明 TRIP9/TRIP10 分路", 
           "仍存在『共用 TRIP9』注释" if stale_comment else "注释已同步",
           "FAIL" if stale_comment else "PASS",
           "代码行为正确、注释过期，属文档一致性问题")

    # ---------------- TZ 检查 ----------------
    dom = "tz"
    exp_tz = exp["tz"]
    ck.cmp("A03-TZ-01", dom, "TZ 项数 = 6 路 INV + 4 路 PV = 10", exp_tz["item_count"]["value"], len(tz))
    inv_pwms = exp_tz["inv"]["pwms"]
    inv_entries = [e for e in tz if e["u16PwmId"] in inv_pwms]
    ok_inv = all(
        e["u16DcaTrip"] == exp_tz["inv"]["dca_trip"]
        and e["u16DcbTrip"] == exp_tz["inv"]["dcb_trip"]
        and e["u16DcaEvent"] == exp_tz["inv"]["dca_event"]
        and e["u16DcbEvent"] == exp_tz["inv"]["dcb_event"]
        and e["u16DcaAction"] == exp_tz["inv"]["dca_action"]
        and e["u16DcbAction"] == exp_tz["inv"]["dcb_action"]
        for e in inv_entries)
    ck.cmp("A03-TZ-02", dom, "六路逆变：DCAH←TRIP9、DCBH←TRIP10、两侧 FORCE_LOW",
           True, ok_inv and len(inv_entries) == 6,
           detail="实际条目: %s" % [{e["u16PwmId"]: (e["u16DcaTrip"], e["u16DcbTrip"])} for e in inv_entries])

    exp_pv = exp_tz["pv"]["value"]
    act_pv = {e["u16PwmId"]: e["u16DcaTrip"] for e in tz if e["u16PwmId"] in exp_pv}
    ck.cmp("A03-TZ-03", dom, "四路 PV：A 侧各自独立 TRIP，B 侧不接", exp_pv, act_pv,
           detail="PV 条目 DCB 应为 %s" % exp_tz["pv"]["dcb_trip"])
    pv_dcb_ok = all(e["u16DcbTrip"] == exp_tz["pv"]["dcb_trip"]
                    and e["u16DcbAction"] == "HW_PWM_TZ_DO_NOTHING"
                    for e in tz if e["u16PwmId"] in exp_pv)
    if not pv_dcb_ok:
        ck.checks[-1]["verdict"] = "FAIL"

    bus_has_tz = any(e["u16PwmId"] == "HW_PWM_ID7" for e in tz)
    ck.add("A03-TZ-04", dom, "BUS(EPWM8) 具备专属硬件 TZ 源", "RULED_DESIGN",
           exp_tz["bus_tz_present"]["value"], "存在 BUS TZ 条目" if bus_has_tz else "无 BUS TZ 条目",
           "PASS" if bus_has_tz else "FAIL",
           "F03：EPWM8 没有专属 TRIP 源。StartSystem() 用 Hw_PwmStop(HW_PWM_ID7) 写 TZFRC.OST，"
           "由硬件锁存封住两路门极（main.c:193-195；注释说明 DC 互补路径会把 A 强制低变成 B 高，"
           "AQCSFRC 不足以封锁）。在允许 BUS 解锁运行之前，必须落实并验证其硬件保护路径；"
           "本步骤只登记，不修")

    enc_ok = bool(re.search(r"DCAHTRIPSEL\.all\s*=\s*\(Uint16\)\(1U\s*<<\s*\(u16Trip\s*-\s*1U\)\)", pwm_drv_raw)) \
        and bool(re.search(r"DCAHCOMPSEL\s*=\s*\(Uint16\)\(u16Trip\s*-\s*1U\)", pwm_drv_raw))
    ck.add("A03-TZ-05", dom, "DCAHTRIPSEL 按 1<<(TRIPn-1) 编码、COMPSEL 选对应比较通道",
           exp_tz["dcahtripsel_encoding"]["basis"],
           exp_tz["dcahtripsel_encoding"]["value"],
           "驱动按该编码写入" if enc_ok else "驱动编码与期望不符",
           "PASS" if enc_ok else "FAIL",
           "TRM TZ 子模块：DCAH 是或门，TRIPINPUTn 选位、COMPSEL 选参与比较的那一路（Hw_PwmDriver.c:502-512）")

    codes = {}
    for name, val in re.findall(r"#define\s+(HW_PWM_TZ_\w+)\s+(\d+)U", read_text(src, "Drivers/inc/Hw_PwmDriver.h")):
        codes[name] = int(val)
    ck.cmp("A03-TZ-06", dom, "TZ 动作码语义与器件编码一致", exp_tz["tz_action_codes"]["value"], codes)

    # ---------------- ePWM 检查 ----------------
    dom = "pwm"
    exp_p = exp["pwm"]

    def inv_ok(d):
        return (d["u16Period"] == exp_p["inv"]["period"]
                and tok_str(d["u16CtrMode"]) == exp_p["inv"]["ctr_mode"]
                and d["u16CmpA"] == exp_p["inv"]["cmpa"]
                and d["u16DeadTime"] == exp_p["inv"]["deadtime_counts"])

    inv_ids = exp_p["inv"]["pwms"]
    ck.cmp("A03-PWM-01", dom, "六路逆变：周期 2500 UPDOWN、CMPA 2500、死区 80",
           True, all(inv_ok(pwm_by_id[i]) for i in inv_ids),
           detail="实际: %s" % {i: (pwm_by_id[i]["u16Period"], tok_str(pwm_by_id[i]["u16CtrMode"]),
                                   pwm_by_id[i]["u16CmpA"], pwm_by_id[i]["u16DeadTime"]) for i in inv_ids})

    ck.cmp("A03-PWM-02", dom, "逆变 AQ：CAU=SET、CAD=CLEAR",
           {"CAU": exp_p["inv"]["aq_cau"], "CAD": exp_p["inv"]["aq_cad"]},
           {"CAU": pwm_by_id["HW_PWM_ID0"]["stAqA"][2], "CAD": pwm_by_id["HW_PWM_ID0"]["stAqA"][3]})

    pv_ids = exp_p["pv"]["pwms"]
    ck.cmp("A03-PWM-03", dom, "四路 PV：周期 4999、UP 计数",
           {"period": exp_p["pv"]["period"], "mode": exp_p["pv"]["ctr_mode"]},
           {"period": pwm_by_id[pv_ids[0]]["u16Period"],
            "mode": tok_str(pwm_by_id[pv_ids[0]]["u16CtrMode"])},
           detail="周期沿用修复中确定的 4999；相位单独判（A03-PWM-09）")
    ck.cmp_design("A03-PWM-09", dom, "四路 PV 交错相位 0/1250/2500/3750",
                  {"value": exp_p["pv"]["phases"],
                   "basis": exp_p["pv"].get("phases_basis", "PENDING")},
                  {i: pwm_by_id[i]["u16Phase"] for i in pv_ids},
                  detail="值一致只说明与表自洽；90° 交错、对齐点以及是否随 TBPRD 重算，"
                         "缺少独立设计依据（01_current_logic.md:35 要求严格 20 kHz 时重算相位）")

    bus = pwm_by_id[exp_p["bus"]["pwm"]]
    ck.cmp("A03-PWM-04", dom, "BUS：周期 2500 UPDOWN、独立同步（FREE）",
           {"period": exp_p["bus"]["period"], "mode": exp_p["bus"]["ctr_mode"],
            "sync": exp_p["bus"]["sync_mode"]},
           {"period": bus["u16Period"], "mode": tok_str(bus["u16CtrMode"]),
            "sync": tok_str(bus["u16SyncMode"])})
    ck.cmp_design("A03-PWM-10", dom, "BUS 初始占空比 CMPA = 1250",
                  {"value": exp_p["bus"]["cmpa"],
                   "basis": exp_p["bus"].get("cmpa_basis", "PENDING")},
                  bus["u16CmpA"],
                  detail="BUS 尚无平衡 PI 与占空比下发策略（05_fix_status.md:16），50% 只是"
                         "初始值，没有独立依据可判为批准值")

    gload_flags = {i: pwm_by_id[i]["bGload"] for i in inv_ids + pv_ids}
    forces_off = bool(re.search(r"GLDCTL\.bit\.GLD\s*=\s*0U", pwm_drv_raw)) and \
        bool(re.search(r"GLDCTL\.bit\.GLDMODE\s*=\s*0U", pwm_drv_raw))
    ck.add("A03-PWM-05", dom, "全局装载（GLD）按表启用并生效", "RULED_DESIGN",
           "bGload=1 的组应真正启用全局装载",
           {"table_bGload": gload_flags, "driver_forces_GLD_off": forces_off},
           "FAIL" if (forces_off and any(gload_flags.values())) else "PASS",
           "F09：表意图=跨模块同步装载；驱动在配置尾部（Hw_PwmDriver.c:178-182）无条件清 "
           "GLD/GLDMODE/OSHTLD，全局装载在运行时被关闭。CMP 并非无阴影直写——CMPCTL "
           "LOADAMODE=CTR_ZERO、SHDWAMODE=SHADOW（:265-268）仍保留本地 ZERO 装载，"
           "缺的是组内同步与同周期一致性")

    uses_syncsel = "u16SyncOutSel" in pwm_drv_raw
    ck.add("A03-PWM-06", dom, "驱动使用表的 SYNCOSEL 字段", "RULED_DESIGN",
           "应据 u16SyncOutSel 写 TBCTL.SYNCOSEL",
           "驱动未引用该字段（SYNCOSEL 由 PWM ID 推导）" if not uses_syncsel else "已引用",
           "FAIL" if not uses_syncsel else "PASS",
           "表字段与运行时行为不一致；当前推导规则固化在 Hw_PwmDriver.c:224-240")

    xlink_ok = bool(re.search(r"EPWMXLINK\.bit\.CMPALINK\s*=\s*pstPwmCfgItem->u16PwmId", pwm_drv_raw)) \
        and bool(re.search(r"EPWMXLINK\.bit\.CMPBLINK\s*=\s*pstPwmCfgItem->u16PwmId", pwm_drv_raw))
    ck.add("A03-PWM-07", dom, "EPWMXLINK 的 CMPA/CMPB 链指回自身",
           exp_p["ePwmXlink_self"]["basis"], exp_p["ePwmXlink_self"]["value"],
           "CMPALINK/CMPBLINK = u16PwmId" if xlink_ok else "未指回自身",
           "PASS" if xlink_ok else "FAIL",
           "驱动 Hw_PwmConfigGlobalLoad 首两行；若保留复位值 0 会镜像到 EPWM1 的 CMPA")

    ck.add("A03-PWM-08", dom, "逆变死区时间", "PENDING",
           "PENDING（80 TBCLK，实际 ns 取决于 TBCLK 频率与器件规格）",
           "80 计数", "PENDING", "需项目所有者按功率器件/驱动电路裁定")

    # ---------------- 汇总 ----------------
    counts = {"PASS": 0, "FAIL": 0, "PENDING": 0, "INFO": 0}
    domains = {}
    for c in ck.checks:
        counts[c["verdict"]] += 1
        d = domains.setdefault(c["domain"], {"PASS": 0, "FAIL": 0, "PENDING": 0, "INFO": 0})
        d[c["verdict"]] += 1

    fails = [c["id"] for c in ck.checks if c["verdict"] == "FAIL"]
    pend = [c["id"] for c in ck.checks if c["verdict"] == "PENDING"]
    a03_verdict = "未通过" if fails else ("部分通过（含待裁定项）" if pend else "通过")

    report = {
        "meta": {
            "step": "01",
            "item": "A03",
            "generated_at": datetime.datetime.now().isoformat(timespec="seconds"),
            "source_commit": args.rev or "unknown (caller did not pass --rev)",
            "source_tree": str(src),
            "source_file_sha256": dict(sorted(SOURCE_HASHES.items())),
            "expected_config": "docs/review/steps/evidence/01/expected_config.json",
            "statement": "静态配置核对：比对配置表与驱动写入逻辑，不验证板上时序、极性或闭环行为。",
        },
        "summary": {"counts": counts, "by_domain": domains,
                    "a03_verdict": a03_verdict, "fail_items": fails, "pending_items": pend},
        "checks": ck.checks,
        "actuals": {
            "adc_soc": adc_ch,
            "adc_modules": [{k: d[k] for k in ADC_MOD_FIELDS} for d in adc_mod],
            "xbar_input": xbar_in, "xbar_trip_mux": xbar_mux, "xbar_trip_out": xbar_out,
            "tz": tz, "pwm": [{k: d[k] for k in PWM_FIELDS} for d in pwm],
        },
    }
    write_text_lf(outdir / "config_replay_report.json",
                  json.dumps(report, ensure_ascii=False, indent=2))

    # CSV：ADC SOC 表（16 行 × 4 模块）
    lines = ["module,soc,entry_index,trig,acqps,ch,used,socctl_word_hex"]
    for m_i, md in enumerate(adc_mod):
        for soc in range(16):
            idx = soc * md["u16SocStride"] + md["base_offset"]
            e = adc_ch[idx] if idx < len(adc_ch) else None
            if e is None:
                continue
            word = (e["trig"] << 20) | (e["ch"] << 15) | e["acqps"]
            used = 1 if (e["trig"], e["acqps"], e["ch"]) != (0, 0, 0) else 0
            lines.append("%s,SOC%d,%d,%d,%d,%d,%d,0x%08X" % (
                tok_str(md["u16AdcId"]), soc, idx, e["trig"], e["acqps"], e["ch"], used, word))
    write_text_lf(outdir / "config_adc_soc_table.csv", "\n".join(lines) + "\n")

    lines = ["pwm,dca_trip,dcb_trip,dca_event,dcb_event,dca_action,dcb_action"]
    for e in tz:
        lines.append(",".join([e[k] for k in TZ_FIELDS]))
    write_text_lf(outdir / "config_tz_table.csv", "\n".join(lines) + "\n")

    lines = ["pwm,type,period,cmpa,deadtime,phase,ctr_mode,sync_mode,gload"]
    for d in pwm:
        lines.append("%s,%s,%d,%d,%d,%d,%s,%s,%d" % (
            tok_str(d["u16PwmId"]), tok_str(d["u16PwmType"]), d["u16Period"], d["u16CmpA"],
            d["u16DeadTime"], d["u16Phase"], tok_str(d["u16CtrMode"]),
            tok_str(d["u16SyncMode"]), d["bGload"]))
    write_text_lf(outdir / "config_pwm_table.csv", "\n".join(lines) + "\n")

    print(json.dumps({"summary": report["summary"], "counts": counts}, ensure_ascii=False, indent=2))
    for c in ck.checks:
        if c["verdict"] != "PASS":
            print("[%s] %s %s | expected=%s | actual=%s" % (
                c["verdict"], c["id"], c["title"], c["expected"], c["actual"]))
    return 0 if not fails else 2


if __name__ == "__main__":
    sys.exit(main())

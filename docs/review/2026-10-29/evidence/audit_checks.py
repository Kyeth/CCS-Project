from pathlib import Path
import re, csv, json, math, hashlib, shutil

root=next(p for p in Path(__file__).resolve().parents if (p/'main.c').exists() and (p/'Broad').is_dir())
out=root/'docs/review/2026-10-29'
ev=out/'evidence'
ev.mkdir(parents=True,exist_ok=True)
def clean(s): return re.sub(r'/\*.*?\*/|//[^\n]*','',s,flags=re.S)
s=clean((root/'Broad/src/DrvCfg_EP15K.c').read_text(encoding='utf-8-sig'))
body=re.search(r'stAdcChCfgItem\[\]\s*=\s*\{(.*?)\};',s,re.S).group(1)
items=[tuple(map(int,x)) for x in re.findall(r'\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}',body)]
assert len(items)==64
rows=[]
for module in range(4):
    for soc in range(16):
        expected=items[4*soc+module]
        actual=items[soc+module]
        rows.append(dict(adc='ABCD'[module],soc=soc,intended_index=4*soc+module,actual_index=soc+module,
            intended_trigger=expected[0],actual_trigger=actual[0],intended_acqps=expected[1],actual_acqps=actual[1],
            intended_channel=expected[2],actual_channel=actual[2],match=expected==actual))
with (ev/'adc_register_replay.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)

def crc_bit(data):
    c=0xffff
    for b in data:
        c^=b
        for _ in range(8): c=(c>>1)^ (0xa001 if c&1 else 0)
    return c
crc_src=clean((root/'Platform/Common/src/Crc16.c').read_text(encoding='utf-8-sig'))
table=[int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{4})U',crc_src.split('};')[0])]
crc_table_ok=len(table)==256 and all(v==crc_bit([i])^crc_bit([0]) for i,v in enumerate(table))
# Linearity above compares a table step to a zero-start shift difference.
def crc_actual(data):
    c=0xffff
    for b in data: c=(c>>8)^table[(c^b)&255]
    return c
checks={
 'scope':'Static configuration replay and calculations, NOT DSP/peripheral emulation or hardware tests.',
 'adc_table_items':len(items),'adc_mismatches':sum(not r['match'] for r in rows),
 'intended_socb_count':sum(r['intended_trigger']==6 for r in rows),
 'actual_socb_count':sum(r['actual_trigger']==6 for r in rows),
 'adca_soc15_actual':list(items[15]),'adca_soc9_actual':list(items[9]),
 'frequency_hz_assuming_tbclk_100mhz':{'inv_bus':100e6/(2*2500),'pv':100e6/(5000+1)},
 'deadband_ns_assuming_halfcycle_0':80/100e6*1e9,
 'acquisition_ns_at_sysclk_200mhz':{a:(a+1)/200e6*1e9 for a in (30,60,120)},
 'watchdog_ms_wdps7_at_intosc1_10mhz':256*512*64/10e6*1000,
 'filter_alpha006_fc_hz_at_20khz':20000/(2*math.pi)*math.acos(1-.06**2/(2*(1-.06))),
 'crc_table_matches_bit_reference':crc_table_ok,
 'crc_123456789':hex(crc_actual(b'123456789')),
 'low_active_or_then_low_trip':[{'oc_n':a,'sc_n':b,'trip_current':not(a|b),'trip_required':not(a&b)} for a in (0,1) for b in (0,1)],
 'excluded_can_suspect':'TRM CAN_IF1CMD.Control says CMD.TXRQST overrides IF1MCTL.TXRQST; current request is valid.'
}
checks['modbus_tables']={}
for source in ('ArmPointTable.c','CommRegTable.c'):
    content=clean((root/'User_App/Comm/src'/source).read_text(encoding='utf-8-sig'))
    for m in re.finditer(r'const\s+ST_MB_REG_ITEM\s+(\w+)\s*\[[^\]]*\]\s*=\s*\{(.*?)\};',content,re.S):
        entries=[(int(a),t) for a,t in re.findall(r'\{\s*(\d+)U?,\s*(MB_VAL_\w+)',m.group(2))]
        overlaps=[]
        for (a,t),(b,_) in zip(entries,entries[1:]):
            if a+(2 if t in ('MB_VAL_U32','MB_VAL_I32','MB_VAL_F32') else 1)>b: overlaps.append([a,b])
        checks['modbus_tables'][m.group(1)]={'entries':len(entries),'overlap_or_unsorted':overlaps}
checks['modbus_word_order']={'declared_enum_HIGH_FIRST':1,'preprocessor_undefined_identifier_evaluates_to':0,'selected_branch':'low word first'}
assert checks['crc_123456789']=='0x4b37'
(ev/'audit_results.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2),encoding='utf8')
paths=[]
for d in ('Broad','Drivers','Platform','User_App','CMD'):
    paths += [p for p in (root/d).rglob('*') if p.suffix.lower() in ('.h','.c','.cla','.cmd')]
paths += [root/'main.c',root/'.cproject',root/'.ccsproject',root/'.project']
with (ev/'source_manifest.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.writer(f);w.writerow(['path','bytes','lines','sha256'])
    for p in sorted(paths):
        if p.is_file():
            data=p.read_bytes();w.writerow([p.relative_to(root).as_posix(),len(data),len(data.splitlines()),hashlib.sha256(data).hexdigest()])
for name in ('build.log','link.log','build.py'):
    shutil.copyfile(root/'tmp/audit'/name,ev/name)
shutil.copyfile(root/'tmp/audit/buildcheck/Debug/EP15K_2026.map',ev/'EP15K_2026.map')
print(json.dumps(checks,ensure_ascii=False,indent=2))
print('manifest files:',len(paths))

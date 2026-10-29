from pathlib import Path
import re, shutil, subprocess
root=Path.cwd(); dest=root/'tmp/audit/buildcheck'; dest.mkdir(exist_ok=True)
for name in ['Broad','Drivers','Platform','User_App','device','CMD']:
 shutil.copytree(root/name,dest/name,dirs_exist_ok=True,ignore=shutil.ignore_patterns('Debug','Release','.git'))
shutil.copy2(root/'main.c',dest/'main.c')
for p in (root/'Debug').rglob('*'):
 if p.is_file() and (p.suffix in ['.mk','.opt'] or p.name=='makefile'):
  t=dest/'Debug'/p.relative_to(root/'Debug'); t.parent.mkdir(parents=True,exist_ok=True); t.write_bytes(p.read_bytes())
with (root/'tmp/audit/build.log').open('w',encoding='utf-8') as f:
 r=subprocess.run(['E:/CCS/ccs/utils/bin/gmake.exe','-B','-j4','EP15K_2026.out'],cwd=dest/'Debug',stdout=f,stderr=subprocess.STDOUT)
print('build exit',r.returncode)

"""Check literal runtime /Game object references against on-disk packages.
Not a cook manifest verifier; formatted/dynamic references require separate review.
"""
from pathlib import Path
import re, json, argparse
root=Path(__file__).resolve().parents[3]
a=argparse.ArgumentParser();a.add_argument('--output',required=True);args=a.parse_args()
out=Path(args.output)
if out.exists(): raise SystemExit('Refusing to overwrite evidence')
rows=[]
for source in sorted((root/'03_UnrealProject/MyProject/Source/MyProject').glob('*.cpp')):
 for number,line in enumerate(source.read_text(encoding='utf-8-sig').splitlines(),1):
  for ref in re.findall(r'"(/Game/[^"\\]+)"',line):
   if '%' in ref or '.' not in ref: continue
   package=ref.split('.')[0]
   path=root/'03_UnrealProject/MyProject/Content'/(package.removeprefix('/Game/')+'.uasset')
   rows.append(dict(source=str(source.relative_to(root)),line=number,reference=ref,exists=path.is_file()))
report=dict(passed=bool(rows) and all(r['exists'] for r in rows),count=len(rows),references=rows,limitation='Literal object package existence only, including guarded review code; does not prove cooked discovery or formatted references')
out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2))
print(json.dumps({k:v for k,v in report.items() if k!='references'}))
raise SystemExit(0 if report['passed'] else 1)

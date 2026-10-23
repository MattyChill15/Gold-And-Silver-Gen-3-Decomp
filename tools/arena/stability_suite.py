#!/usr/bin/env python3
"""Run ROM gates headlessly on independent disposable cores; retain failures."""
import importlib.util
import json
from pathlib import Path
import sys
import time
import traceback
import lab

ROOT = Path(__file__).resolve().parents[2]
results = []
result_name = 'suite-' + '-'.join(sys.argv[1:]) + '.json'
for name in sys.argv[1:]:
    core = lab.Lab.__new__(lab.Lab)
    core.runner = None
    core.live_speed = None
    core.mode = 'lab'
    def call(q):
        began = time.monotonic()
        try:
            result = {'headless': True} if q['action'] == 'view' else core.handle(q)
            return dict(ok=True, result=result, elapsed_s=time.monotonic()-began)
        except Exception as error:
            return dict(ok=False, error=str(error), elapsed_s=time.monotonic()-began)
    lab.call = call
    began = time.monotonic()
    entry = dict(suite=name, ok=False)
    try:
        spec = importlib.util.spec_from_file_location(name, ROOT/'tests/arena'/f'{name}.py')
        module = importlib.util.module_from_spec(spec)
        arguments = sys.argv
        sys.argv = [str(ROOT/'tests/arena'/f'{name}.py')]
        try:
            spec.loader.exec_module(module)
            if hasattr(module, 'main'):
                module.main()
        finally:
            sys.argv = arguments
        entry.update(ok=True, checks=len(getattr(module, 'checks', [])))
    except Exception:
        entry['error'] = traceback.format_exc()
        print(entry['error'], flush=True)
    finally:
        entry.update(seconds=round(time.monotonic()-began, 2), rom=getattr(core, 'sha', None))
        if core.runner:
            core.raw('quit', reply=False)
            core.runner.wait(timeout=5)
        results.append(entry)
        (lab.WORK/'results'/result_name).write_text(json.dumps(results, indent=2)+'\n')
        print('SUITE', json.dumps(entry), flush=True)
if not all(r['ok'] for r in results):
    sys.exit(1)

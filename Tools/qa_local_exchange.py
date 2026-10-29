"""Exercise the real Unreal game-thread transport against a local FAKE model.
No downloads, weights, external network, player-save writes, or global settings.
Only a new child game and a temporary 127.0.0.1 server belong to this run.
"""
from __future__ import annotations
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import re
import subprocess
import threading
import time
import uuid

from model_request_contract_v1 import validate_request


def digest(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def sources(root: Path) -> list[dict]:
    paths = [root / 'AIHeroes.uproject']
    for directory in ('Source', 'Config'):
        paths += sorted((root / directory).rglob('*'))
    return [dict(path=p.relative_to(root).as_posix(), bytes=p.stat().st_size, sha256=digest(p))
            for p in paths if p.is_file()]


def saves(root: Path) -> list[dict]:
    return [dict(name=p.name, bytes=p.stat().st_size, sha256=digest(p))
            for p in sorted((root / 'Saved/SaveGames').glob('*.sav'))]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--engine', type=Path, default=Path(r'C:\Program Files\Epic Games\UE_5.8\Engine'))
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('Windows Unreal execution required; no synthetic PASS outside Windows.')
    root = args.root.resolve()
    busy = subprocess.check_output(['powershell.exe', '-NoProfile', '-Command',
        '@(Get-CimInstance Win32_Process | Where-Object {$_.Name -in @(\'UnrealEditor.exe\',\'UnrealEditor-Cmd.exe\',\'cl.exe\') -or ($_.Name -eq \'dotnet.exe\' -and $_.CommandLine -match \'UnrealBuildTool\')}).Count'], text=True).strip()
    if busy != '0':
        raise RuntimeError('Existing engine/build active; no additional game launched.')
    evidence = root / 'Saved/QA' / ('LocalExchange_' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ') + '_' + uuid.uuid4().hex[:8])
    evidence.mkdir(parents=True)
    print('LOCAL_EXCHANGE_EVIDENCE=' + str(evidence), flush=True)
    before, save_before = sources(root), saves(root)
    dll = root / 'Binaries/Win64/UnrealEditor-AIHeroes.dll'
    dll_hash = digest(dll)
    (evidence / 'source-manifest.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    (evidence / 'player-saves.json').write_text(json.dumps(save_before, indent=2), encoding='utf-8')
    events: list[dict] = []
    failures: list[str] = []
    lock = threading.Lock()
    allowed = {'qa-valid','qa-redirect','qa-overflow','qa-duplicate','qa-tools','qa-bad-model','qa-stale','qa-timeout','qa-cancel'}

    class FakeModel(BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.0'
        def log_message(self, *_):
            pass
        def do_GET(self):
            with lock:
                failures.append('Unexpected GET or followed redirect')
            self.send_error(400)
        def do_POST(self):
            self.connection.settimeout(2.0)
            try:
                lengths = self.headers.get_all('Content-Length') or []
                if self.path != '/v1/chat/completions' or len(lengths) != 1:
                    raise ValueError('Wrong path or ambiguous request framing')
                length = int(lengths[0])
                if not 0 < length <= 131072:
                    raise ValueError('Unbounded request')
                data = self.rfile.read(length)
                if len(data) != length:
                    raise ValueError('Truncated request')
                request = json.loads(data)
                model = request['model']
                if model not in allowed or request['stream'] is not False or request['max_tokens'] != 96:
                    raise ValueError('Unknown scenario or unbounded generation')
                prompt = json.loads(request['messages'][1]['content'])
                state = validate_request(json.dumps(prompt['snapshot'], ensure_ascii=True).encode())
                ids = [c['candidate_id'] for c in prompt['candidates']]
                if ids != ['c' + str(i) for i in range(2 + len(state['legal_orders']))]:
                    raise ValueError('Menu order drift')
                if {n['id'] for n in state['visible_nodes']} != {'HQ','FOREST','ORE'}:
                    raise ValueError('Fog leak or unexpected fixture')
                if 'tools' in request or 'tool_choice' in request:
                    raise ValueError('Unexpected tool authority')
                if request['response_format']['schema']['properties']['candidate_id']['enum'] != ids:
                    raise ValueError('Schema/menu drift')
                with lock:
                    events.append({'model':model,'body_bytes':len(data),'menu_choices':len(ids),'fog_filtered':True})
                if model in {'qa-valid','qa-stale'}:
                    time.sleep(0.35)
                if model == 'qa-timeout':
                    time.sleep(5.5)
                if model == 'qa-cancel':
                    time.sleep(2.0)
                if model == 'qa-redirect':
                    self.send_response(302)
                    self.send_header('Location',f'http://127.0.0.1:{self.server.server_port}/redirect-target')
                    self.send_header('Content-Length','0')
                    self.end_headers()
                    return
                if model == 'qa-overflow':
                    self.send_response(200)
                    self.send_header('Content-Type','application/json')
                    self.send_header('Content-Length','32769')
                    self.end_headers()
                    return
                inner = {'request_id':state['request_id'],'candidate_id':'c2' if len(ids)>2 else 'c0'}
                message = {'role':'assistant','content':json.dumps(inner,separators=(',',':'))}
                if model == 'qa-tools':
                    message['tool_calls'] = []
                envelope = {'model':'wrong-model' if model=='qa-bad-model' else model,
                    'choices':[{'index':0,'finish_reason':'stop','message':message}]}
                body = json.dumps(envelope,separators=(',',':'))
                if model == 'qa-duplicate':
                    body = body.replace('"model":','"model":"qa-duplicate","model":',1)
                raw = body.encode('ascii')
                self.send_response(200)
                self.send_header('Content-Type','application/json')
                self.send_header('Content-Length',str(len(raw)))
                self.end_headers()
                # Actual fragmentation, not only an in-memory parser fixture.
                middle = len(raw)//2
                self.wfile.write(raw[:middle]); self.wfile.flush(); time.sleep(0.01)
                self.wfile.write(raw[middle:]); self.wfile.flush()
            except (BrokenPipeError,ConnectionResetError,ConnectionAbortedError,TimeoutError):
                pass  # Expected when the game cancels or enforces its deadline.
            except Exception as exc:
                with lock:
                    failures.append(type(exc).__name__ + ': ' + str(exc))
                try:
                    self.send_error(400)
                except OSError:
                    pass

    server = ThreadingHTTPServer(('127.0.0.1',0),FakeModel)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    receipt = {'schema':'aiheroes.local-exchange-qa.v1','status':'PARTIAL_OR_FAIL',
        'real_model_loaded':False,'checkpoint_attested':False,'external_network_calls':0,
        'synthetic_loopback_server':True,'save_writes_requested':0,'android_verified':False}
    child = None
    try:
        log = evidence / 'game.log'
        cmd = [str(args.engine / 'Binaries/Win64/UnrealEditor.exe'),str(root/'AIHeroes.uproject'),
            '-game','-unattended','-NullRHI','-NoSplash','-NoP4','-NoSound',
            '-AIHeroesLocalExchangeSmoke',f'-AIHeroesMockPort={server.server_port}',f'-abslog={log}']
        child = subprocess.Popen(cmd,cwd=root,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        receipt['pid'] = child.pid
        try:
            code = child.wait(timeout=120)
        except subprocess.TimeoutExpired:
            # This exact child belongs to this run. Never target other processes.
            child.terminate(); child.wait(timeout=10)
            raise RuntimeError('Owned game timed out; no PASS')
        receipt['game_exit'] = code
        text = log.read_text('utf-8-sig',errors='replace')
        cases = re.findall(r'\[LOCAL EXCHANGE CASE\] index=(\d+) pass=1 frames=(\d+) choice_valid=(\d+)',text)
        passed = re.search(r'\[LOCAL EXCHANGE SMOKE\] PASS cases=10 offline_preserved=1 readiness_off=1 strategy_commands=0 save_calls=0 max_pending_frames=(\d+)',text)
        if code != 0 or not passed or '[LOCAL EXCHANGE SMOKE] FAIL' in text:
            raise RuntimeError('Real engine scenario did not finish cleanly')
        if [int(x[0]) for x in cases] != list(range(10)) or int(passed[1]) < 2:
            raise RuntimeError('Missing/duplicate cases or no ticking evidence')
        with lock:
            captured, errors = list(events), list(failures)
        counts = Counter(e['model'] for e in captured)
        if errors or counts['qa-valid'] != 2 or any(counts[x] != 1 for x in allowed-{'qa-valid','qa-cancel'}) or counts['qa-cancel'] not in (0,1):
            raise RuntimeError('Mock server evidence mismatch: '+repr((counts,errors)))
        after, save_after = sources(root), saves(root)
        receipt.update(source_files=len(before),source_unchanged=after==before,
            player_saves=len(save_before),saves_unchanged=save_after==save_before,
            dll_stable=digest(dll)==dll_hash,dll_sha256=dll_hash,
            cases=[dict(index=int(a),frames=int(b),choice_valid=bool(int(c))) for a,b,c in cases],
            loopback_requests=len(captured),server_events=captured,server_errors=errors,
            game_log_bytes=log.stat().st_size,game_log_sha256=digest(log))
        if before != after or save_before != save_after or digest(dll) != dll_hash:
            raise RuntimeError('Source, save or binary drift; reconcile before PASS')
        receipt['status'] = 'PASS_WINDOWS_ASYNC_LOOPBACK_MOCK_ONLY'
    except Exception as exc:
        receipt['error'] = type(exc).__name__ + ': ' + str(exc)
    finally:
        server.shutdown(); server.server_close(); thread.join(timeout=2)
        receipt['observed_utc'] = datetime.now(timezone.utc).isoformat()
        receipt['evidence'] = str(evidence)
        out = evidence / 'receipt.json'
        raw = (json.dumps(receipt,ensure_ascii=True,indent=2)+'\n').encode()
        with out.open('xb') as handle:
            handle.write(raw)
        if out.read_bytes()!=raw:
            raise RuntimeError('Receipt readback failed')
        print(json.dumps(receipt,ensure_ascii=True,indent=2),flush=True)
    return 0 if receipt['status']=='PASS_WINDOWS_ASYNC_LOOPBACK_MOCK_ONLY' else 2

if __name__ == '__main__':
    raise SystemExit(main())

# Local model exchange v1 - nonblocking loopback transport

Status: PASS_WINDOWS_ASYNC_LOOPBACK_MOCK_ONLY. Real inference and checkpoint attestation remain NOT VERIFIED.
Scope: the main five-site AIHeroesGameMode. No procedural expedition integration.

## What changed

StartLocalModelExchange(port, modelAlias, explicitLocalOptIn, error) builds the
existing fog-filtered request and starts one nonblocking socket. Actor Tick
pumps the transport before battle/replay early returns. The UI/offline adviser
is not replaced: LastHint, LastEvent, dossier and campaign remain unchanged.
GetLocalModelExchangeResult returns the existing guarded reply verdict.
Production readiness remains false. A valid response may be decoded but cannot
be advertised as verified model advice or execute any strategy command.

A new request cancels the old socket and request. Closing the client connection
does not guarantee a separately running model server stops its generation.
CancelLocalModelAdvice also cancels the transport. BeginPlay, allowed LoadCampaign attempts, EndPlay and
Destroyed invalidate the exchange. Closed old sockets cannot submit a callback
into a new request: this implementation has no background UObject callbacks.

## Deliberately restricted transport

Only direct numeric 127.0.0.1, explicit port 1024..65535, and a restricted model
alias are accepted. There is no URL/hostname, DNS, proxy, redirect follower,
authentication, cookie jar, process spawn, server startup or model download.
No network activity occurs in ordinary play or ordinary unit tests.

The implementation uses Unreal FSocket with nonblocking mode. Each Tick can
send at most 8192 bytes and receive at most 8192 bytes. There is no Wait, Sleep,
worker join or HTTP flush in the game thread. Header scans and JSON parsing are
bounded, but this is not a measured Android/frame-rate performance guarantee.

The wall-clock budget is 4 seconds, checked on each game Tick, not an idle
socket timer. The reply session retains its separate 5-second age limit.
If game ticking is suspended, expiration is processed on the next tick; no
claim of an OS-enforced hard deadline is made. Backwards/nonfinite clock fails.

A restricted HTTP/1.0 request asks for a non-streamed chat completion at
/v1/chat/completions with Connection: close and Accept-Encoding: identity.
Only a 200 response with an explicit bounded Content-Length and JSON content
is supported. Chunked, close-delimited, compressed, redirected and malformed
responses fail closed. This is a limited local profile, NOT a general HTTP/1.1
client. Compatibility with real llama.cpp builds must be demonstrated rather
than inferred from the endpoint name.

Bounds: 8192 header bytes, 32768 body bytes, 131072 request bytes. This first
wire profile accepts ASCII JSON (Unicode strings may use JSON escapes).
Arbitrary Unicode free-form model prose is not displayed. The inner reply
remains two restricted ASCII string fields: request_id, candidate_id.

## Request and response checks

The chat request contains the existing exported snapshot and an indexed menu:
c0 NoAction, c1 Advice, c2+ legal orders in precisely the exported order.
A JSON schema requests only those IDs, temperature 0, stream false, 96 output
tokens. Grammar is a hint to the model server, not a substitute for validation.

A token pass rejects duplicate object keys (including escaped equivalents),
more than 12 container levels and more than 4096 JSON tokens before object
construction. The envelope must echo the configured model alias, contain one
choice at index 0, assistant role, finish_reason stop, and valid inner content.
Tool/function calls, refusals, wrong request IDs and out-of-menu IDs fail.
Alias equality is not proof of the loaded checkpoint or parameter count.
The existing reply gate re-checks engine state, host context, session and model
readiness. Choosing a legal action is not evidence of strategic quality.

## Test surfaces

AIHeroes.LocalExchange: seven offline automation groups. They cover endpoint
and opt-in preflight, real exported menu/fog, all HTTP response prefixes,
ambiguous framing, model identity/finish reason, duplicate/deep JSON and clock/
cancellation preflight. They do not open sockets.

Tools/qa_local_exchange.py launches one disposable game and a fake local HTTP
server on an OS-assigned loopback port. The fake deliberately exercises valid
fragmented responses, redirect, oversized length, duplicate JSON, tool calls,
wrong model alias, intervening EndTurn, timeout and cancellation; a final valid
response checks recovery. Real engine ticks and the existing reply gate run.
Two synthetic choices must decode but remain rejected for unattested weights.
No actual inference is performed. The test script only owns its launched game
and server, and never terminates unrelated programs.

The QA records source and DLL hashes and checks all existing .sav bytes before
and after. It does not create or load user saves. Test-player actions are one
normal protocol purchase and one normal EndTurn; transport actions are zero.

## Running and rollback

Use Tools/verify_local_exchange.ps1 after other engine/build sessions finish.
It builds/runs all gameplay tests and the existing four-export schema scenario,
then runs the real loopback-mock scenario and checks the same source/binary.

Rollback: apply the inverse of this scoped integration, using baselines under
Saved/QA/LocalExchangeWork_20261029. Do not overwrite later edits to shared files.
Rebuild and re-run the previous verification before claiming rollback complete.
No commit, push, global config, canon or persistent-memory changes are included.

## Primary references checked 2026-10-29

- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Sockets/FSocket
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Sockets/FSocket/Recv
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json/TJsonReader
- https://www.rfc-editor.org/rfc/rfc9112.html (framing; this client is intentionally restricted)
- https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md

Next gates: actual server/checkpoint attestation, real inference, explicit UI
opt-in and useful advice display, server build compatibility, Android/device QA.

## Verified evidence

All 208 gameplay tests passed (including seven new offline transport groups); ten actual loopback scenarios passed in a disposable Unreal game. Four real exports and twelve negative schema controls passed. Four existing saves and the complete source/binary snapshot stayed unchanged. Evidence: Saved/QA/LocalExchangeValidation_20261029T132436201Z/receipt.json.

Incidents: the first full run passed 207/208 tests. A concurrently added expedition-adviser test used discovered-count < 7 as a proxy for no second battle. It was replaced with whole reflected-state comparison plus movement/HP negative controls; gameplay was not changed. An initial network attempt correctly refused to run while another engine process was active.

The loopback address is not proof that a separately installed server cannot relay data externally. No real server or model was installed or attested by this change.

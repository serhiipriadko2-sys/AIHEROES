# Local-model reply gate v1

Status: PASS_SCOPED_WINDOWS. Engine automation and export/schema checks passed; no actual LLM inference or UI integration claimed.
Scope: main five-site AIHeroesGameMode, engine-owned candidate menu, transient response correlation.

## Player-facing boundary

The normal ASK HERO action remains the offline advisor. No HTTP service, model weights,
process spawn, file access or automatic strategy command is added by this slice.
A model selection is never a permission to move, purchase, attack or save.
The production GameMode always sets local/remote checkpoint readiness to false.
Consequently a well-formed synthetic selection can pass syntax/correlation but cannot
be presented as a verified model proposal. Positive acceptance is tested only using
explicit synthetic readiness in disposable test fixtures, not by changing production flags.

## Single-source menu

FAIHeroesModelRequest::Build has an optional OutMenu output. It is created in the same
pass as legal_orders: c0 = NoAction, c1 = Advice, c2+ = the exported orders in their
stable engine-defined order. Failure clears both outputs. The emitted request JSON
schema and existing exported field values are unchanged.
The complete proposal, target, cost explanation and source metadata are reconstructed
inside the engine; arbitrary model explanation text is never copied into gameplay/UI.

## Engine entry points

- BeginLocalModelAdvice(OutJson, Error): game-thread-only request preparation, no inference.
- CompleteLocalModelAdvice(SelectionJson): read-only verdict, no LastHint or campaign mutation.
- CancelLocalModelAdvice(): clear a pending request without changing the campaign.

A fresh FGuid token identifies each request. A separate session token rotates on
BeginPlay and on an allowed LoadCampaign attempt. The transient object is not serialized.
A newer begin attempt invalidates the older one, including when the new begin fails.
An unrelated late reply cannot consume the newer pending request.
A matching reply is single-use even if invalid, stale or rejected for missing readiness.
Malformed replies with no trustworthy correlation remain rejected until cancellation,
expiration checked on completion, or replacement. No background timer/transport exists.

## Narrow selection wire form

The input is the normalized INNER model choice, not an HTTP response or a full
aiheroes.hero-result.v1 envelope:

```json
{"request_id":"0123456789abcdef0123456789abcdef","candidate_id":"c2"}
```

The parser accepts exactly these two unescaped ASCII string fields in either order,
with JSON whitespace outside strings, maximum 256 TCHARs. Nested values, duplicate
keys, extra fields, escape sequences, non-ASCII strings and noncanonical candidate IDs
are rejected. This intentionally supports a restricted JSON subset, not arbitrary JSON.
A future transport adapter must validate HTTP status, model binding, finish_reason,
tool/refusal fields, response size, opt-in and server provenance separately, then
normalize the bounded choice. Merely calling this method is not proof of inference.

## Staleness and timeout

The session stores the original request bytes and an exact bounded host context.
Before accepting a selection it re-exports the current snapshot with the same tokens
and compares exact strings. This detects changes even if the legacy CRC revision is
accidentally unchanged. The GameMode supplies its full CampaignFingerprint as host
context to cover campaign fields not exported to a model. That context is never sent.
Current model readiness is checked through the existing AIProposalGate after decoding.
Production lifetime = 5 seconds; core limit = 30 seconds. Expiry uses monotonic
FPlatformTime in GameMode, with injected numeric times only in tests. The boundary
Now >= Deadline rejects; nonfinite values and backwards time also reject.
No event-loop dispatch or callback thread policy is implemented by this synchronous gate.

## Verification

Automation prefix: AIHeroes.ModelReply (nine groups).
The synthetic cases include 512 unrelated reply IDs, invalid grammar, menu parity,
state/host-context drift, cancellation, session rotation, single-use behavior,
clock bounds and actual disposable GameMode calls with model readiness OFF.

Run after any other engine/build session has finished:

```powershell
powershell -NoProfile -File Tools/test_model_manifest_arrays.ps1
powershell -NoProfile -File Tools/verify_model_request.ps1
```

The second command compiles, runs all gameplay tests and checks four real game exports
against the original Python bridge schema. It is not model inference.
Add -Visual only for an additional three-resolution map graphical run.

## Save-manifest regression

Windows PowerShell 5.1 can return a JSON array as one pipeline object. The prior
@(... | ConvertFrom-Json) wrapper created one nested array and produced a false drift
with null fields. @((... | ConvertFrom-Json)) enumerates the parsed top-level result.
The fix changes one expression, not the actual save bytes or the expected hash values.
The regression tool exercises the exact inspected production assignment on fixtures.

## Evidence and non-claims

Primary references, consulted 2026-10-29:
- https://github.com/PowerShell/PowerShell/issues/3424
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/FGuid
- https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine
- https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md

Not implemented here: production HTTP/bridge response adapter, real checkpoint identity,
actual LLM tactical combat, procedural expedition export, Android/device testing.
No claim that choosing a legal action is strategically optimal.
Rollback: reverse only this slice using Saved/QA/ModelReplyWork_20261029 baselines;
never replace shared files wholesale if later concurrent edits exist.

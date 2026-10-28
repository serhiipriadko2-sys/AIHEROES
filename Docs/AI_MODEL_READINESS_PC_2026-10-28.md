# AI HEROES — Windows model readiness / evidence, 2026-10-28

Surface: DESKTOP-ITUAMNF, C:\github\AIHEROES.
Scope: a read-only capability inspection; NO model download, installation,
network provider activation, credential read or PC settings change occurred.

## Actual readings (23:13 Windows local time, snapshot)

GPU: NVIDIA GeForce RTX 3070.
GPU total video memory: 8192 MiB.
GPU video memory reported free during the reading: 6682 MiB.
GPU compute capability: 8.6.

Windows total physical memory: approximately 32620 MiB.
Windows free physical memory during the reading: approximately 8253 MiB.
The free values are instantaneous and cannot be treated as reserved capacity.

Commands: nvidia-smi --query-gpu=name,memory.total,memory.free,compute_cap
--format=csv,noheader; Win32_OperatingSystem memory query;
Get-Command ollama,llama-server,llama-cli,nvidia-smi.

Command-discovery result:
- nvidia-smi was available at C:\Windows\system32\nvidia-smi.exe.
- ollama: NOT FOUND on PATH.
- llama-server: NOT FOUND on PATH.
- llama-cli: NOT FOUND on PATH.

Absence on PATH is not proof that no executable or models exist elsewhere.
No filesystem-wide search for private models was performed.
No working LLM weights, checkpoint file, adapter or inference response was
verified in this session. The in-game 1B/3B/8B labels are still model slots,
not parameters of a loaded neural network.

## First safe inference experiment

Start with a single locally chosen and properly licensed small checkpoint
and a deliberately isolated inference service. Do NOT allow the model to
execute Unreal console commands or access arbitrary PC files.

1. Obtain separately authorized runtime/model, verify model name, license,
   publisher, checkpoint hash, tokenizer and quantization.
2. Measure available memory with Unreal actually running, not only idle PC.
   Capture latency, VRAM peak, RAM peak, thermal, CPU/GPU and process crashes.
3. Use an explicitly versioned structured advisory response: ADVICE,
   PROPOSE_ORDER or NO_ACTION, target ID, observed day/revision, origin,
   model tier and short explanatory text (<=512 UTF-16 units).
4. Route every response through FAIHeroesAIProposalGate with engine-owned
   snapshot. It rejects stale/invalid inputs, model source mismatch and
   unauthorized remote inference. The gate never executes a game order.
5. For REMOTE_MODEL require explicit opt-in first; allow disabling it.
   Never send local documents, OS paths, API keys, personal data, source
   code or the original SoT30 files as model input.
6. Compare against the deterministic offline policy and do not introduce
   new combat RNG through text generation. GameEngine remains authoritative.
7. Only after measurable PC QA consider Android NDK packaging and separate
   model size/battery/temperature/runtime tests.

A nominal 8B model may require much more GPU memory after context and the
Unreal renderer are accounted for; its suitability for this 8GiB GPU
was NOT demonstrated. CPU fallback is possible conceptually but its
practical latency is unmeasured. Avoid claims of availability/quality until
a real test returns a verified checkpoint ID and performance receipt.

## Acceptance boundaries

LOCAL-GATE BUILT/TESTED does not imply INFERENCE INSTALLED, RUNTIME
CONNECTED, CHECKPOINT LOADED, PLAYER CONSENT, or ANDROID READY.

No automatic downloads, purchases, configuration changes or deployments.
Metrics without concrete inference inputs: computation unavailable.

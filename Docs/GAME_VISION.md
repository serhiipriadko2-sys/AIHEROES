# AIHEROES — concept / vertical slice

## Hook
The player commands a 3D strategic world: a base, resource nodes, neutral creatures,
recruitment, turn economy and decisions. The HERO is an AI companion.
The player chooses where to go and whom to recruit; the hero autonomously
executes tactical battles and explains the outcome.

## Two independent progress tracks
1. Game stats: stamina, strength, army size, economy, base upgrades.
2. Cognitive progression: TRACE -> SIFT -> PLANNING -> GUARD -> COUNCIL;
   separate from changing the underlying LLM checkpoint.

The initial prototype intentionally uses a deterministic rules-based brain.
It does NOT claim that a real 1B/3B/8B model runs inside Unreal.
Later a replaceable inference adapter will connect actual model checkpoints.
On Android, large checkpoints may need a remote inference option.

## Vertical slice: definition of playability
- Perspective (not isometric) 3D camera: WASD, Q/E rotate, mouse wheel zoom.
- A starter base and a small 7x7 world with ore/energy/data/neutral/enemy nodes.
- Select a node, recruit units, end a turn, collect nodes.
- Ask the hero for advice before committing a decision.
- Resolve battles autonomously using the hero's AI combat policy.
- Earn data fragments, unlock protocols, open higher model *slots*.
- Visible labels clearly distinguish real inference from simulated progression.

## Guardrails
No Heroes III characters, art, trademark, soundtrack or map replication.
No direct LLM control over Unreal engine commands. Game state is authoritative;
the advisor proposes bounded decisions, the validator accepts/rejects them.
Offline fallback must keep combat playable on Windows and Android.

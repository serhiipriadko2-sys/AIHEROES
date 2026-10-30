"""Static wiring contract only; not a performance or gameplay benchmark."""
from pathlib import Path

root=Path(__file__).resolve().parents[3]
hud=(root/'Source/AIHeroes/Private/AIHeroesBattleHUD.cpp').read_text(encoding='utf-8-sig')
assert 'FAIHeroesBattleAftermath::Build(' not in hud, 'HUD must not replay/verify the battle each frame'
assert 'G.GetLiveBattleAftermathView()' in hud, 'HUD must read the immutable prepared result'
print('AFTERMATH_HUD_CACHE_STATIC_PASS')

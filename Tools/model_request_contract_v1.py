"""Bounded engine-to-model data, strict JSON and host-authored proposals.

The engine, NOT this module or the model, owns fog visibility and route legality.
This module only accepts an allowlisted snapshot exported by that trusted host.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import math
import re
from typing import Any

MAX_JSON_BYTES = 65_536
MAX_DEPTH = 12
MAX_NODES = 64
MAX_STACKS = 12
MAX_ORDERS = 64
REQUEST_SCHEMA = "aiheroes.hero-request.v1"
IDENTIFIER = re.compile(r"[A-Za-z][A-Za-z0-9_-]{0,47}\Z")
TOKEN = re.compile(r"[a-f0-9]{32}\Z")
MODEL_ALIAS = re.compile(r"[A-Za-z0-9][A-Za-z0-9._/-]{0,95}\Z")
NODE_TYPES = {"Headquarters", "ForestMine", "OreMine", "Outpost", "Ruins"}
GOALS = {"AssessSelected", "Explore", "PreserveArmy"}


class ContractError(ValueError):
    """A public stable error code, never a copy of untrusted content."""


def require(ok: bool, code: str) -> None:
    if not ok:
        raise ContractError(code)


def _pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        require(key not in result, "DUPLICATE_JSON_KEY")
        result[key] = value
    return result


def _constant(_: str) -> None:
    raise ContractError("NONFINITE_JSON")


def _number(token: str, floating: bool = False) -> int | float:
    require(len(token) <= 32, "JSON_NUMBER_SIZE")
    value = float(token) if floating else int(token)
    require(not floating or math.isfinite(value), "NONFINITE_JSON")
    return value


def strict_loads(raw: bytes) -> Any:
    require(type(raw) is bytes and 0 < len(raw) <= MAX_JSON_BYTES, "JSON_SIZE")
    try:
        text = raw.decode("utf-8", "strict")
        # Bound nesting BEFORE asking the standard parser to allocate containers.
        depth = 0
        in_string = escaped = False
        for char in text:
            if in_string:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == '"':
                    in_string = False
            elif char == '"':
                in_string = True
            elif char in "[{":
                depth += 1
                require(depth <= MAX_DEPTH, "JSON_DEPTH")
            elif char in "]}":
                depth -= 1
        return json.loads(text, object_pairs_hook=_pairs, parse_constant=_constant,
                          parse_int=_number, parse_float=lambda t: _number(t, True))
    except (UnicodeError, json.JSONDecodeError, RecursionError) as exc:
        raise ContractError("INVALID_JSON") from exc


def canonical(value: Any) -> bytes:
    try:
        raw = json.dumps(value, ensure_ascii=True, sort_keys=True,
                         separators=(",", ":"), allow_nan=False).encode("ascii")
    except (TypeError, ValueError, RecursionError) as exc:
        raise ContractError("INVALID_JSON_VALUE") from exc
    require(0 < len(raw) <= MAX_JSON_BYTES, "JSON_SIZE")
    return raw


def exact(obj: Any, keys: set[str]) -> dict[str, Any]:
    require(type(obj) is dict and set(obj) == keys, "UNEXPECTED_FIELDS")
    return obj


def integer(value: Any, low: int, high: int) -> None:
    require(type(value) is int and low <= value <= high, "INTEGER_RANGE")


def identifier(value: Any) -> None:
    require(type(value) is str and IDENTIFIER.fullmatch(value) is not None,
            "IDENTIFIER")


def _stack(stack: Any) -> None:
    exact(stack, {"unit_id", "unit_health", "total_health", "attack", "defense", "initiative"})
    identifier(stack["unit_id"])
    integer(stack["unit_health"], 1, 500_000)
    integer(stack["total_health"], 0, 500_000)
    for name in ("attack", "defense", "initiative"):
        integer(stack[name], 0, 100_000)


def _army(army: Any) -> None:
    require(type(army) is list and len(army) <= MAX_STACKS, "ARMY_SIZE")
    for stack in army:
        _stack(stack)


def validate_request(raw: bytes) -> dict[str, Any]:
    s = strict_loads(raw)
    exact(s, {"schema", "request_id", "session_id", "day", "state_revision",
              "protocol_tier", "model_slot_tier", "moves_left", "battle_active",
              "goal", "selected_node", "resources", "player_army", "visible_nodes",
              "legal_orders"})
    require(s["schema"] == REQUEST_SCHEMA, "REQUEST_VERSION")
    for key in ("request_id", "session_id"):
        require(type(s[key]) is str and TOKEN.fullmatch(s[key]) is not None, "REQUEST_TOKEN")
    integer(s["day"], 1, 1_000_000)
    integer(s["state_revision"], 1, 0xFFFFFFFF)
    integer(s["protocol_tier"], 0, 3)
    integer(s["model_slot_tier"], 0, 2)
    integer(s["moves_left"], 0, 4)
    require(type(s["battle_active"]) is bool, "BATTLE_FLAG")
    require(type(s["goal"]) is str and s["goal"] in GOALS, "GOAL")
    exact(s["resources"], {"gold", "wood", "ore"})
    for value in s["resources"].values():
        integer(value, 0, 2_147_483_647)
    _army(s["player_army"])
    require(type(s["visible_nodes"]) is list and len(s["visible_nodes"]) <= MAX_NODES,
            "NODE_COUNT")
    known: dict[str, Any] = {}
    for node in s["visible_nodes"]:
        exact(node, {"id", "type", "owned", "guards"})
        identifier(node["id"])
        key = node["id"].casefold()
        require(key not in known, "DUPLICATE_NODE")
        require(type(node["type"]) is str and node["type"] in NODE_TYPES, "NODE_TYPE")
        require(type(node["owned"]) is bool, "OWNERSHIP_FLAG")
        _army(node["guards"])
        require(not (node["owned"] and any(g["total_health"] > 0 for g in node["guards"])),
                "OWNED_GUARD_CONFLICT")
        known[key] = node
    if s["selected_node"] is not None:
        identifier(s["selected_node"])
        require(s["selected_node"].casefold() in known, "UNKNOWN_SELECTION")
        require(known[s["selected_node"].casefold()]["id"] == s["selected_node"],
                "NONCANONICAL_NODE_ID")
    require(type(s["legal_orders"]) is list and len(s["legal_orders"]) <= MAX_ORDERS,
            "ORDER_COUNT")
    seen: set[tuple[str, str]] = set()
    for order in s["legal_orders"]:
        exact(order, {"order", "target_node", "cost"})
        require(type(order["order"]) is str and order["order"] in {"MoveToNode", "AttackNode"},
                "ORDER_KIND")
        identifier(order["target_node"])
        target = known.get(order["target_node"].casefold())
        require(target is not None, "UNKNOWN_TARGET")
        require(target["id"] == order["target_node"], "NONCANONICAL_NODE_ID")
        integer(order["cost"], 0, 4)
        require(order["cost"] <= s["moves_left"], "UNAFFORDABLE_ORDER")
        key = (order["order"], order["target_node"])
        require(key not in seen, "DUPLICATE_ORDER")
        seen.add(key)
        hostile = not target["owned"] and any(g["total_health"] > 0 for g in target["guards"])
        require((order["order"] == "AttackNode") == hostile, "GUARD_ORDER_CONFLICT")
        if hostile:
            require(target["type"] != "Headquarters" and
                    any(u["total_health"] > 0 for u in s["player_army"]), "ATTACK_FORBIDDEN")
    # Nonzero protocol still cannot grant new transport or gameplay authority.
    require(not s["legal_orders"] or (not s["battle_active"] and s["protocol_tier"] >= 1),
            "ORDER_PHASE_LOCK")
    return s


@dataclass(frozen=True)
class Binding:
    """Operator-configured endpoint alias, NOT an attestation of model weights."""
    model_alias: str
    slot_tier: int

    def validate(self) -> None:
        require(type(self.model_alias) is str and MODEL_ALIAS.fullmatch(self.model_alias) is not None,
                "MODEL_ALIAS")
        integer(self.slot_tier, 0, 2)


def fingerprint(s: dict[str, Any]) -> str:
    return hashlib.sha256(canonical(s)).hexdigest()


def candidates(s: dict[str, Any]) -> list[dict[str, Any]]:
    values = [
        {"candidate_id": "c0", "kind": "NoAction", "order": "None", "target_node": None,
         "explanation": "No strategic action proposed. Player remains in control."},
        {"candidate_id": "c1", "kind": "Advice", "order": "None", "target_node": None,
         "explanation": "Review the visible map and army before deciding. No action executed."},
    ]
    for item in s["legal_orders"]:
        verb = "Consider attacking" if item["order"] == "AttackNode" else "Consider moving to"
        values.append({"candidate_id": f"c{len(values)}", "kind": "ProposeOrder",
                       "order": item["order"], "target_node": item["target_node"],
                       "explanation": f"{verb} {item['target_node']}; engine-quoted cost {item['cost']} moves. "
                                      "This is not a victory forecast. Player confirmation required."})
    return values


def build_payload(s: dict[str, Any], binding: Binding) -> bytes:
    choices = candidates(s)
    schema = {"type": "object", "properties": {
        "request_id": {"type": "string", "enum": [s["request_id"]]},
        "candidate_id": {"type": "string", "enum": [v["candidate_id"] for v in choices]}},
        "required": ["request_id", "candidate_id"], "additionalProperties": False}
    # Session, state token and slot metadata are host-only. No files or free-text prompt enter.
    visible = {key: s[key] for key in ("day", "protocol_tier", "moves_left", "goal",
                                      "selected_node", "resources", "player_army", "visible_nodes")}
    visible.update(request_id=s["request_id"], candidates=choices)
    return canonical({"model": binding.model_alias, "stream": False, "temperature": 0,
                      "max_tokens": 128, "messages": [
                          {"role": "system", "content":
                           "Choose one candidate for this turn-based strategy game. These are suggestions, "
                           "not executable orders. Use only the visible data. Legal does not mean a "
                           "winning battle. When uncertain, choose c0 or c1. Return ONLY an object with "
                           "request_id copied exactly and candidate_id chosen from the supplied list. "
                           "Never add prose, tools, resources, commands, or fields."},
                          {"role": "user", "content": canonical(visible).decode("ascii")}],
                      "response_format": {"type": "json_schema", "json_schema": {
                          "name": "aiheroes_candidate", "strict": True, "schema": schema}}})


def decode_choice(raw: bytes, s: dict[str, Any], binding: Binding) -> dict[str, Any]:
    envelope = strict_loads(raw)
    require(type(envelope) is dict and envelope.get("model") == binding.model_alias, "MODEL_MISMATCH")
    choices = envelope.get("choices")
    require(type(choices) is list and len(choices) == 1 and type(choices[0]) is dict,
            "CHOICE_COUNT")
    choice = choices[0]
    require(choice.get("finish_reason") == "stop", "INCOMPLETE_GENERATION")
    message = choice.get("message")
    require(type(message) is dict and message.get("role") == "assistant", "MESSAGE_ROLE")
    require(not message.get("tool_calls") and not message.get("function_call") and
            not message.get("refusal"), "UNEXPECTED_TOOL_OR_REFUSAL")
    content = message.get("content")
    require(type(content) is str, "MESSAGE_CONTENT")
    try:
        selected = strict_loads(content.encode("utf-8", "strict"))
    except UnicodeError as exc:
        raise ContractError("INVALID_JSON") from exc
    exact(selected, {"request_id", "candidate_id"})
    require(selected["request_id"] == s["request_id"], "REQUEST_MISMATCH")
    require(type(selected["candidate_id"]) is str, "UNKNOWN_CANDIDATE")
    by_id = {v["candidate_id"]: v for v in candidates(s)}
    require(selected["candidate_id"] in by_id, "UNKNOWN_CANDIDATE")
    # Explanation and all order fields are reconstructed locally, never copied from model prose.
    return by_id[selected["candidate_id"]]

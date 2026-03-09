# Coding Style & Naming Conventions Reference

This document captures naming conventions, coding style preferences, and general development practices.
It is based on the Sucker Punch C++ Hungarian notation standard, adapted for Python and other languages.

---

## Core Philosophy

- Readability and debuggability first, then correctness, then ease of writing new code.
- Consistent environments produce fewer bugs.
- Names should communicate *type*, *scope*, and *semantic meaning* simultaneously.
- Prefer declarative, reproducible environments over imperative ones.
- Understand the "why" behind technical choices; don't blindly follow best practices.

---

## Hungarian Notation — The Tag System

Variables follow the pattern: `[scope][prefix][Tag][Detail][Suffix]`

### Scope Prefixes

| Prefix | Meaning |
|--------|---------|
| *(none)* | Local variable |
| `g_` | File-static (module-private) global in Python; global in C++ |
| *(none)* | Module-level globals intended for import by other modules (Python) |
| `s_` | Static scope (C++) |
| `m_` | Class/struct member (C++ only — omit in Python; `self.` provides this context) |

### Prefix Values (prepended before the Tag)

| Prefix | Meaning |
|--------|---------|
| `p` | Pointer (C/C++) |
| `c` | Count |
| `i` | Index |
| `r` | Ratio |
| `u` | Unsigned; also tag for float 0..1 |
| `d` | Delta / change in value (not rate of change) |
| `a` | C-style array |
| `f` | Boolean flag (also standalone tag) |

### Standard Tags

| Tag | Type |
|-----|------|
| `b` | byte (u8) |
| `ch` | char |
| `wch` | wide char (wchar_t aka WCH) |
| `str` | string |
| `g` | float (generic or unknown) |
| `s` | float (scalar or x/y/z component) |
| `u` | float 0..1 |
| `su` | float -1..1 |
| `f` | bool |
| `n` | int (generic; prefer a more specific tag when possible) |
| `t` | datetime / Arrow instance |
| `dT` | duration / timedelta (delta time) |
| `deg` | degrees |
| `rad` | radians |
| `pos` | point/position |
| `vec` | vector (direction, not position) |
| `path` | filesystem path |
| `obj` | complex/opaque dict (e.g., raw JSON/YAML data) |

### Container Tags & Prefixes

| Tag | Type |
|-----|------|
| `l` | list (Python) |
| `a` | raw array (C/C++) |
| `ary` | templated array (C++) |
| `mp` | dict/map with well-defined keys (typically string→value) |
| `obj` | opaque dict (slurped from JSON/YAML) |
| `set` | set |
| `circ` | circular buffer |
| `dl` | doubly-linked list |
| `sl` | singly-linked list |
| `hash` | hash table |

**Capitalization rule:** Use camelCase to separate prefixes, tags, and suffixes within a name.
Capitalize the first letter of each tag/suffix after the initial prefix(es).

Examples with single-char tags:
- Array of floats: `aG` (not `ag`)
- Index into float array: `iG` (not `ig`)
- Count of floats: `cG` (not `cg`)
- Pointer to char: `pCh` (not `pch`)

Examples with multi-char tags:
- Array of chars: `aChLine` (not `achLine`)
- Count of chars: `cChLine` (not `cchLine`)
- Pointer to char at end: `pChEnd` (not `pchEnd`)

Examples with container prefixes:
- Array of pointers: `aPFoo`
- Array of arrays: `aAFoo`
- Map from string to int: `mpStrInt`
- Map from enum to string: `mpEnumStr`
- Set of strings: `setStr`

**Semantic over implementation:** prefer `mpEnumValue` over `tplData`, `lEnum` over `lMember`.

### Variable Name Examples

```python
strName         # local string
lStrNames       # local list of strings
mpStrInt        # dict mapping str → int
setStr          # set of strings
tCreated        # datetime instance
dTElapsed       # duration
cItems          # count of items
iItem           # index into item list
fEnabled        # bool flag
pathOutput      # filesystem path
objConfig       # raw dict from YAML/JSON
```

### Suffixes

| Suffix | Meaning |
|--------|---------|
| `Min` | minimum value or index |
| `Max` | maximum value or one-past-last index |
| `Mic` / `Mac` | min/max of a subrange within a larger container |
| `First` / `Last` | inclusive first/last index |
| `Cur` | current value |
| `Prev` / `Next` | previous / next value |
| `Src` / `Dst` | source / destination |

---

## Class and Type Naming

### Python

| Pattern | Usage |
|---------|-------|
| `CClassName` | Regular class (tag in comment if name > 6 chars) |
| `SClassName` | Struct-like class (data container, Pydantic model, dataclass) |
| `IClassName` | Interface / abstract base class |
| `clsEnum` | Variable holding a class/type object (Hungarian `cls` prefix) |
| `EnumNAME` | Enum type — values use `ENUMNAME_ValueName` style |

Comment the tag on every class, even if the tag equals the class name:

```python
class CProxyFile:  # tag = prxf
    ...

class SDocumentArgs:  # tag = doca
    ...
```

### Enum Naming

```python
from enum import IntEnum

class COLOR(IntEnum):  # All caps type name
    COLOR_Red = 0
    COLOR_Green = 1
    COLOR_Blue = 2
    COLOR_Max = 3
    COLOR_Min = 0
    COLOR_Nil = -1
```

- Bit-flag enums use `F` as the first letter: `FBUTTON`
- Instances that may hold multiple flags use `grf` prefix: `grfbutton`
- State enums use `S` as last letter: `BUTTONS`
- Classification enums use `K` as last letter: `COLORК`
- "Nil" sentinel values = -1

---

## Function and Method Naming

Pattern: `[ReturnTypeTag]VerbNoun(...)`

- Omit return tag if void.
- Use `Is` verb for bool queries: `FIsEmpty()`
- Use `Set` for setters; legacy code may use `Get` for getters (no longer preferred).
- Functions that may fail and return bool: `FTryWhatever()`
- Factory / lookup functions return their tag: `StrName()`, `PathOutput()`

```python
def FIsEmpty(self) -> bool: ...
def StrName(self) -> str: ...
def SetName(self, strName: str) -> None: ...
def PropagateChanges(self) -> None: ...

def FTryFindFile(strFile: str) -> tuple[bool, Path | None]: ...
def StrFromColor(color: COLOR) -> str: ...
def MpStrDocaLoad(pathYaml: Path) -> dict[str, SDocumentArgs]: ...
```

---

## Python-Specific Conventions

### Type Hints

- **Always** annotate function parameters and return types.
- Use `from __future__ import annotations` to enable forward references without quotes (add short comment: `# Forward refs without quotes`).
- Prefer `X | None` over `Optional[X]` (Python 3.10+).
- Use `from typing import TYPE_CHECKING` for import-only-at-type-check-time imports.

### Class Members (Python)

- No `m_` prefix — `self.` provides the same context.
- All member fields should be commented if non-obvious.
- Pydantic `S`-prefixed models should be `frozen=True` by default.
- Use `model_config = ConfigDict(frozen=True)`.

### Module Layout

- Prefer `src/` layout with `pyproject.toml`.
- Use `uv` for dependency management.
- Use `devenv` / `direnv` for automatic environment activation.
- Editable installs (`uv sync`) for development; no need to re-sync on source changes.
- CLI entry points defined in `[project.scripts]` in `pyproject.toml`.
- Use relative imports (`from .common import helpers`) within a package.

### Data Loading

- Use **Pydantic** for structured YAML/JSON loading into typed models.
- `S`-prefix structs are the natural fit for Pydantic models.
- `obj` tag for raw unvalidated dicts; `S`-model instances once validated.
- Inject dict keys into models explicitly before construction when needed.

### Enums in Python

- Subclass `IntEnum` (or `IntEnum0` for auto-start-at-zero).
- Provide `_Max`, `_Min`, `_Nil = -1` sentinels.
- Use `EnumTuple` / typed mapping classes for enum-indexed tables.

---

## C++-Specific Conventions (Summary)

- Tabs (4 spaces), not spaces.
- `m_` prefix for all class/struct members.
- No `enum class` — use plain enums to allow integer conversion.
- Use `TWEAKABLE` instead of `static const` for magic numbers.
- `ASSERT` / `CASSERT` / `VERIFY` for correctness checks.
- Use `nullptr` not `NULL`.
- `const` on read-only methods and pointed-to objects; not on primitive locals.
- C++ casts (`static_cast`, `reinterpret_cast`) — never C-style casts.
- `(void)FTrySomething();` to explicitly ignore return values.
- `override` keyword on all overridden virtual functions.
- No exceptions, no RTTI, no STL.

---

## Comments

- Use `//` not `/* */`.
- Comments get a blank line before and after (except end-of-line comments and comments at the top of a newly-indented block).
- Write comments as you code; don't wait until submission.
- `NOTE(username)` for non-obvious but correct decisions.
- `BB(username)` for areas needing improvement.
- All struct/class members should be commented unless trivially obvious.
- Header: one-to-two-line class description; one-liner per non-obvious method.
- Don't duplicate what the code already says.

---

## General Development Preferences

### Environment & Tooling

- **Nix/NixOS** for reproducible build environments — preferred despite complexity.
- **devenv + direnv** for per-project environment activation.
- **uv** for Python dependencies.
- **VSCode** as primary editor with carefully managed extensions.
- **PlatformIO** for embedded (ESP32/Feather) development.
- **CMake + Ninja** for C++ builds; `CMakePresets.json` for configuration.
- **Git submodules** (not subtrees) for vendored dependencies that may need upstream contributions.

### Git Workflow

- Clean separation between private build config and upstream-contributable code.
- Stacked branches for complex changes.
- Submodule updates committed in isolation from other changes.
- Fork upstream + submit PR rather than maintaining local patches.
- `git rebase --abort` / `git reset --hard origin/<branch>` for conflict recovery.

### Project Structure (Python)

```
project/
├── src/
│   └── packagename/
│       ├── __init__.py
│       ├── common/       # shared utilities
│       └── commands/     # CLI subcommands
├── tests/
├── pyproject.toml
├── devenv.nix
├── .envrc
└── uv.lock
```

### Coding Patterns

- Prefer **declarative** over imperative.
- Prefer **simple** solutions over complex ones when they meet the need.
- Type safety and explicit interfaces prevent more bugs than they create overhead.
- Comprehensive type hints everywhere.
- Pydantic for all external data validation.
- `from __future__ import annotations` standard in typed files.
- Understand the mechanism, not just the fix.

---

## Quick Reference Card

```
# Variable name anatomy:
# [scope][prefix][Tag][Detail][Suffix]
#
# scope:   g_ = file-private global | (none) = local or exported global
# prefix:  c = count | i = index | f = bool | p = pointer | d = delta
# Tag:     str | n | g | f | t | dT | l | mp | set | obj | path | ...
# Detail:  descriptive words
# Suffix:  Min/Max/Mic/Mac/Cur/Prev/Next/Src/Dst

# Function name anatomy:
# [ReturnTag]VerbNoun
#
# void:           PropagateChanges()
# bool:           FIsEmpty() / FTrySomething()
# str:            StrName()
# dict[str, X]:   MpStrXLoad()
# datetime:       tCreated()

# Type prefixes:
# C = class | S = struct/data | I = interface | (ALL_CAPS) = enum
```
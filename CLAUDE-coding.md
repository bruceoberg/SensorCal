# Coding Style & Naming Conventions

General coding conventions, Hungarian notation, and development practices that apply
across all languages. Language-specific details live in companion files:

- `CLAUDE-coding-c.md` — C++ conventions (full detail)
- `CLAUDE-coding-python.md` — Python conventions and project tooling

---

## Core Philosophy

- Readability and debuggability first, then correctness, then ease of writing new code.
- Consistent environments produce fewer bugs.
- Names should communicate *type*, *scope*, and *semantic meaning* simultaneously.
- Prefer declarative, reproducible environments over imperative ones.
- Prefer simple solutions over complex ones when they meet the need.
- Understand the "why" behind technical choices; don't blindly follow best practices.

---

## Hungarian Notation — The Tag System

Variables follow the pattern: `[scope][prefix][Tag][Detail][Suffix]`

### Scope Prefixes

| Prefix | Meaning | C++ | Python |
|--------|---------|-----|--------|
| *(none)* | Local variable | ✓ | ✓ |
| `g_` | Global / file-private global | ✓ | ✓ (file-private only) |
| `s_` | Static scope | ✓ | — |
| `m_` | Class/struct member | ✓ | — (`self.` serves this role) |

In Python, module-level globals *intended for import* by other modules use no prefix.
Only file-private globals (not exported) use `g_`.

### Prefix Values

| Prefix | Meaning | Notes |
|--------|---------|-------|
| `p` | Pointer | C++ only |
| `c` | Count | |
| `i` | Index | |
| `r` | Ratio | |
| `u` | Unsigned; also tag for float 0..1 | |
| `d` | Delta / change in value (not rate of change) | |
| `a` | C-style array | C++ only |
| `f` | Boolean flag (also standalone tag) | |

### Standard Tags

| Tag | Type | Notes |
|-----|------|-------|
| `b` | byte (`u8`) | |
| `ch` | char | |
| `str` | string | |
| `g` | float | |
| `u` | float 0..1 | |
| `su` | float -1..1 | |
| `f` | bool | |
| `n` | int (generic; prefer a more specific tag when possible) | |
| `t` | datetime / Arrow instance | Python; `t` = time in C++ |
| `dT` | duration / timedelta | Python |
| `deg` | degrees | |
| `rad` | radians | |
| `pos` | point/position | |
| `vec` | vector (direction, not position) | |
| `path` | filesystem path | Python (`pathlib.Path`) |
| `obj` | raw/opaque dict (e.g., unvalidated JSON/YAML) | Python |

### Container Tags

| Tag | Type | Notes |
|-----|------|-------|
| `l` | list | Python |
| `ary` | templated array | C++ |
| `mp` | dict/map with well-defined keys | both |
| `set` | set | both |
| `circ` | circular buffer | C++ |
| `dl` | doubly-linked list | C++ |
| `sl` | singly-linked list | C++ |
| `hash` | hash table | C++ |

**Capitalization rule:** when multiple tags stack, capitalize the first letter of each
additional tag:

```
aryPFoo         # array of pointers
aryAryFoo       # array of arrays
mpStrInt        # map from string to int
mpEnumStr       # map from enum to string
setStr          # set of strings
lStrNames       # list of strings  (Python)
```

**Semantic over implementation:** prefer `mpEnumValue` over `tplData`, `lEnum` over
`lMember`. Names should describe *what* the data means, not how it is stored.

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

`Max` is *not* a valid index — an array with 3 elements has `Max = 3`, `Last = 2`.
Use `Mic`/`Mac` for subrange bounds to avoid confusion with the container's own `Min`/`Max`.

---

## Class and Type Naming

| Prefix | Usage |
|--------|-------|
| `C` | Regular class |
| `S` | Struct-like / data container (Pydantic model, dataclass, plain struct) |
| `I` | Interface / abstract base class |

Always comment the tag, even when it matches the class name:

```cpp
class CProxyFile : public CBasic  // tag = prxf
class CFoo                        // tag = foo
struct SBar                       // tag = bar
```

```python
class CProxyFile:   # tag = prxf
class SDocArgs:     # tag = doca
```

---

## Enum Naming

- Type name in ALL_CAPS.
- Values prefixed with the type name and an underscore: `ENUMNAME_ValueName`.
- Provide `_Nil = -1`, `_Min = 0`, `_Max` (one past last) on non-flag enums.
- Bit-flag enums: `F` as *first* letter — `FBUTTON`. Instances holding multiple flags: `grf` prefix — `grfbutton`.
- State enums: `S` as *last* letter — `BUTTONS`.
- Classification enums: `K` as *last* letter — `COLORK`.

```cpp
enum COLOR : s8
{
    COLOR_Red, COLOR_Green, COLOR_Blue,
    COLOR_Max, COLOR_Min = 0, COLOR_Nil = -1
};
```

```python
class COLOR(IntEnum):
    COLOR_Red = 0; COLOR_Green = 1; COLOR_Blue = 2
    COLOR_Max = 3; COLOR_Min = 0;   COLOR_Nil = -1
```

---

## Function and Method Naming

Pattern: `[ReturnTypeTag]VerbNoun(...)`

- Void functions: omit return tag — `PropagateChanges()`
- Bool queries: `FIs...` — `FIsEmpty()`
- May-fail functions returning bool: `FTry...` — `FTryFindFile()`
- Factory / lookup: return tag as prefix — `StrName()`, `PathOutput()`
- Setters: `Set...` — `SetName()`

```python
def FIsEmpty(self) -> bool: ...
def StrName(self) -> str: ...
def SetName(self, strName: str) -> None: ...
def PropagateChanges(self) -> None: ...
def FTryFindFile(strFile: str) -> tuple[bool, Path | None]: ...
def MpStrDocaLoad(pathYaml: Path) -> dict[str, SDocumentArgs]: ...
```

---

## Comments

- Use `//` not `/* */`.
- Comments get a blank line before and after — except end-of-line comments, and
  comments at the top of a newly-indented block.
- Write comments as you code; don't wait until submission.
- `NOTE(name)` for non-obvious but correct decisions.
- `BB(name)` for areas needing improvement.
- All struct/class members should be commented unless trivially obvious.
- Header/class: one-to-two-line description; one-liner per non-obvious method.
- Don't duplicate what the code already says.

---

## General Development Preferences

### Environment & Tooling

- **Nix/NixOS** for reproducible build environments.
- **VSCode** as primary editor.
- **Git submodules** (not subtrees) for vendored dependencies that may need upstream contributions.
- **direnv** for per-project environment activation. Sometimes with `devenv.sh`; sometimes with a  `shell.nix`.
- **CMake + Ninja** for C++ builds; `CMakePresets.json` for configuration.
- **PlatformIO** for embedded (ESP32/Feather) development.
- **uv** for Python dependencies and lockfiles.

### Git Workflow

- Clean separation between private build config and upstream-contributable code.
- Stacked branches for complex changes.
- Submodule updates committed in isolation from other changes.
- Fork upstream + submit PR rather than maintaining local patches.
- `git rebase --abort` / `git reset --hard origin/<branch>` for conflict recovery.

### Coding Patterns

- Prefer **declarative** over imperative.
- Type safety and explicit interfaces prevent more bugs than they create overhead.
- Understand the mechanism, not just the fix.

---

## Quick Reference

```
Variable anatomy:   [scope][prefix][Tag][Detail][Suffix]

scope:   g_ = file-private global  |  (none) = local or exported global
         m_ = class member (C++ only)  |  s_ = static (C++ only)
prefix:  c = count | i = index | f = bool | p = pointer (C++) | d = delta
Tag:     str | n | g | f | t | dT | l | mp | set | obj | path | ...
Suffix:  Min/Max/Mic/Mac/Cur/Prev/Next/Src/Dst

Function anatomy:   [ReturnTag]VerbNoun

void:           PropagateChanges()
bool:           FIsEmpty() / FTryLoad()
str:            StrName()
dict[str, X]:   MpStrXLoad()

Type prefixes:  C = class | S = struct/data | I = interface | ALL_CAPS = enum
```
# Sucker Punch C++ Coding Standards

## Introduction

This document outlines Sucker Punch's coding standards for the C++ language.

The decisions were made according to how well they satisfied the following questions, in rough order of importance:

1. How easy is it to understand and debug existing code?
2. How easy is it to use existing code in the correct way?
3. How easy is it to write new code which performs correctly?
4. How small are the performance penalties (particularly for the engine)?

When a project tries to adhere to common standards a few good things happen:

- Programmers can go into any code and figure out what's going on.
- New people can get up to speed quickly.
- People new to C++ are spared the need to develop a personal style and defend it to the death.
- People new to C++ are spared making the same mistakes over and over again.
- People make fewer mistakes in consistent environments.

This document leans towards conciseness. The following books are highly recommended for further explanations:

- *C++ Coding Standards*, Herb Sutter and Andrei Alexandrescu
- *Code Complete*, Steve McConnell
- *Effective C++*, Scott Meyers
- *More Effective C++*, Scott Meyers

---

## Basics

Tabs not spaces. Tab stop set to 4 spaces.

---

## Names

Sucker Punch uses a modified Hungarian convention for naming throughout the code base. Although it does simplify (and solidify) some aspects of naming, you still need to spend some time coming up with useful names. If you find all your names could be `Thing` and `DoIt` then you should probably revisit your design.

### Class + Struct

- Classes start with a leading `C` prefix, and mixed case.
- If the class name is long, create a shortened tag to use for variables of that type, and comment it. The general rule of thumb is that if a class name is less than 7 characters long, then no shortened tag is necessary.
- The tag should always be specified in a comment, even if the tag is in fact the same as the class name. See formatting examples below.
- Prefix with `I` if it is an interface class (no data, just pure virtual functions).
- Template classes follow the same rule as non-template classes.
- Structs follow the same rules, except for starting with a leading `S` instead of a leading `C`.

```cpp
class CProxyFile : public CBasic // tag = prxf
{
};

template <typename T>
class CDl<T> // tag = dl
{
    // No tag needed
};

class CFoo // tag = foo
{
    struct SBar // tag = bar
    {
    };
};
```

### Method and Function Names

- Method and function names should be formatted with `[OptionalReturnType]VerbNoun`. If the function returns void, skip the return type.
- Casing on function and method names is the same as a variable with the first letter upper case.
- Use `Is` for the verb on query functions, prefixed by the return type, of course.
- Use `Set` for value-set functions. Note that in early versions of our conventions, we used `Get` for value-get functions, but this is no longer the convention.
- Usually every method and function performs an action, so the name should make clear what it does: `CheckForErrors()` instead of `ErrorCheck()`.
- If a function can fail in normal usage, and returns a Boolean to indicate whether it succeeded, then it should be named `FTryWhatever`.

```cpp
class CFoo
{
public:
    bool       FIsEmpty() const;
    void       PropagateChanges();
    CString    StrName() const;
    void       SetName(CString strName);
};

void     UpdateAllObjects();
bool     FFindFile(CString strFile, CFile ** ppFile);
char *   PChzLookup(OID oid);
CHero *  PHeroLocalPlayer();
SObject* PObjFromINode(int iNode);
```

### Variables

Variables should use the format `[scope][prefix][Tag][details][suffix]`.

#### Scope Values

| Scope | Meaning |
|-------|---------|
| *(none)* | Local scope |
| `g_` | Global scope |
| `s_` | Static scope. Note that `TWEAKABLE` variables have static scope and should use the `s_` prefix. |
| `m_` | Class/struct member scope |

#### Prefix Values

| Prefix | Meaning |
|--------|---------|
| `p` | Pointer |
| `ea` | Pointer to main memory (SPU code distinguishing local vs. main memory addresses) |
| `c` | Count |
| `i` | Index |
| `r` | Ratio |
| `u` | Unsigned. Note that `u` is also a tag, used for "parameter"-style floats between 0 and 1. Also the tag for the first part of a 2D texture coordinate. |
| `d` | Delta or change in value. For a measured or expected change, not a rate of change. So `dcFoo` means "the count of foos changed by this much", not a rate. For the latter, add information to the detail: `cFooPerSecond`. |
| `a` | C-style array |
| `ary` | Templated array |
| `circ` | Circular buffer |
| `dl` | Doubly-linked list |
| `sl` | Singly-linked list |
| `mp` | Array which "maps" one type to another; e.g., `mpFooPChz` maps the enum type `Foo` to a string. Capitalization: upper case for the first letter of each tag: `mpTag1Tag2`. Can be used with C-style arrays or templated arrays. |
| `hash` | Hash table. E.g., `CHash<CString, OID> hashStrOid;` |
| `set` | Set |

Note: if a variable name starts with multiple prefix values, values beyond the first should not have their first letter capitalized. For example, an array of arrays is `aryaryFoo`, and an array of pointers is `arypFoo`.

There are also some prefixes which only really make sense in a particular context:

| Prefix | Meaning |
|--------|---------|
| `s` | Directionless velocity. Typically, `sV = Length(v)`. |
| `dV` | Delta velocity (although we try to use the more specific tag `dv` to mean acceleration — derivative of velocity) |

#### Tag Values

The tag is a shortened version of a class, or the class name itself if the class name is short. The first letter is capitalized if there is a prefix value.

**Standard tags:**

| Tag | Type |
|-----|------|
| `b` | Byte (`u8`) |
| `ch` | Character (`char`) |
| `g` | Float |
| `u` | Float between 0 and 1 |
| `su` | Float between -1 and 1 |
| `s` | Distance |
| `f` | Boolean |
| `t` | Time |
| `n` | Integer. You shouldn't see many bare integers; generally there's a better Hungarian description. |
| `deg` | Degrees |
| `rad` | Radians |
| `pos` | Point/position |
| `vec` | Vector (direction, not position — note the distinction enforced by our math libraries) |
| `x`, `y`, `z` | Single coordinate value on each axis. Combinations allowed: `Xy`, `Xyz`. |
| `normal` | Normalized or normal vector |
| `mat` | Matrix or transform. Use `matWorldToClip` (`AToB`) to signify transform spaces. E.g., `posObjB = posObjA * matObjAToWorld * matWorldToObjB`. |
| `v` | Velocity. Prefixes clarify use: `sV` (magnitude of velocity). By convention, `dv` = acceleration (like gravity), `dV` = instantaneous change in velocity. |
| `w` | Angular velocity. Similar rules as `v`: `dw` and `sW` are angular acceleration/torque and magnitude of angular velocity. |
| `eul` | Set of Euler angles in radians |

For enum types: the name of the enum, all lowercase. `TRAIT` → `trait`, `FBTN` → `fbtn`, `GRFBTN` → `grfbtn`, `INSTANCEK` → `instancek`.

**Less standard tags:**

| Tag | Meaning |
|-----|---------|
| `mp` | "Juice" / Magic Points (Cole and many electric objects) |
| `hp` | Hit Points |
| `dw` | `DWORD` |

**Notes:**
- Use `dX`, `dY`, `dZ` or similar for texel, pixel, voxel dimensions.
- In the rare case that a type follows a full tag, capitalize the first letter of the second type as you would for `mp` (e.g., `lmDT`).

#### Details

Whatever is needed for specifics of the variable.

#### Suffixes

| Suffix | Meaning |
|--------|---------|
| `Min` | Minimum value, or minimum index |
| `Max` | Maximum value, or maximum index (last valid index + 1). `Max` is **not** a valid index — an array with 3 elements has `Max = 3` (2 being the highest valid index). |
| `Mic` | Same as `Min`, but for a subrange of a larger list. (`Mic >= Min` of the container) |
| `Mac` | Same as `Max`, but for a subrange within a larger list. (`Mac <= Max` of the container) |
| `First` | Alternative to `Min` (`First = Min`). Useful when using `Last`. |
| `Last` | Highest valid index (`Last = Max - 1`) |
| `Cur` | Current value |
| `Prev` | Previous value |
| `Next` | Next value |
| `Src` | Source value |
| `Dst` | Destination value |

In short: if you're working with a subrange of a larger container, use `Mic` and `Mac` for its bounds so they don't get confused with the `Min` and `Max` of the container.

#### Exceptions

If you're looping, but not looping over anything, you can use a naked `i` as the iteration variable. Similarly, an iteration count can be a naked `c`.

```cpp
class CFoo
{
    CFixAry<bool, 256> m_aryF;    // Array of flags with class scope.

    static CFoo * s_pFooCur;      // static class scope pointer
};

CDynAry<CFoo *> g_arypFooWorld;   // global array of foos

void ProcessFooWorld()
{
    int cpFoo = g_arypFooWorld.C(); // local count of pointer to foos.

    // static maximum count of pointers to foos.

    static int s_cpFooMax = max(s_cpFooMax, cpFoo);

    if (cpFoo == 0)
    {
        return;
    }

    int cFMin = INT_MAX; // local minimum count of flags
    for (int ipFoo = 0; ipFoo < cpFoo; ++ipFoo)
    {
        CFoo * pFoo = g_arypFooWorld[ipFoo]; // local foo pointer
        if (pFoo != NULL)
        {
            cFMin = min(cFMin, pFoo->m_aryF.C());
        }
    }
}
```

---

## Enums

- Do not use `enum class`. Disallowing integer conversion breaks common enum table use like `mpEnumToPChz`.
- Name the enumerated type in ALL CAPS.
- Name the values using descriptive names prefixed by the enum name and an underscore: `ENUM_ValueName`.
- For bit-flags enums, use `F` as the *first* letter of the enumeration: `FENUM`. Consider using `DEFINE_ENUM_FLAGS` to define helpful functions.
- To indicate that a particular instance of a flags enum may contain multiple bits, prefix with `GRF`: `GRFENUM`.
- Consider using `K` as the *last* letter of the enumeration if it classifies things.
- Consider using `S` as the *last* letter of the enumeration if it names a state.
- Provide `Nil = -1`, `Min = 0`, and `Max` as one past the last element of non-flag enums.
- Consider using `DEFINE_ENUM_INCDEC` to define helpful increment/decrement operators.
- For enums inside classes or namespaces, use the `DECLARE` and `IMPLEMENT_*_OPERATORS` macros.
- Manually specify an integral type if you need the enum to be bigger or smaller than an `int`.
- Forward declare when you need to avoid including a definition in a header file.

```cpp
// NOTE: These are in flux as we upgrade. E.g. DEFINE_ENUM_FLAGS is not implemented yet.

enum COLOR : s8
{
    COLOR_Red,
    COLOR_Orange,
    COLOR_Yellow,
    COLOR_Green,
    COLOR_Blue,
    COLOR_Indigo,
    COLOR_Violet,
    COLOR_RedGreen,

    COLOR_Max,
    COLOR_Min = 0,
    COLOR_Nil = -1
};
DEFINE_ENUM_INCDEC(COLOR, Color);

enum FBUTTON
{
    FBUTTON_None     = 0,
    FBUTTON_X        = 0x1,
    FBUTTON_Square   = 0x2,
    FBUTTON_Triangle = 0x4,
    FBUTTON_Circle   = 0x8,
};
DEFINE_ENUM_FLAGS(FBUTTON);

// Usage:

if (grfbutton & FBUTTON_X)
{
    TRACE("Color is %s", PchzFromColor(COLOR_Red));
}
```

---

## Macros

- Macros are `ALL_CAPS_WITH_UNDERBAR`.
- Macro functions should still follow `VERB_NOUN` naming convention.
- Avoid where possible.

---

## Templates

- In general, follow the standard rules for naming. Template classes should be named the same as concrete classes; template functions should follow the same names as concrete functions.
- Template parameters should be a single capital letter, to make it easy to distinguish between template parameters and variables.
- Provide return parameters for functions.

---

## Classes and Structs

Both classes and structs are supported, but by convention we restrict what language features are used in structs. In structs, these features are **allowed**:

- Methods
- Constructors + destructors

These features should **not** be used in structs:

- Access control — all struct members are public.
- Virtual functions — if a virtual function is necessary, it should be a class.

Other guidelines:

- Structs, classes, and namespaces are "columnified." Nothing else should be. Even for structs et al., don't be a slave to the columns — different sections of the struct, class, or namespace can use different column arrangements.
- Really long type names are allowed to spill over column boundaries. If they do, wrap down to the next line and align the variable in the right column.
- Multiple base types should be split across multiple lines.
- Simple structs/classes without constructors may use class member default values in place of simple constructors. For complex or tunable default values, use a constructor instead. The two should not be intermixed.

### Namespaces

Namespaces are going to be adopted in wider use, but we need to figure out what that means first.

Guidelines when using namespaces:

- Use to collect related groups of `TWEAKABLE` options for a class or system.
- The namespace uses the name of the associated class, minus the leading `C`. If the namespace is not directly associated with a class — which should be unusual — then choose a reasonable name.
- Typically, the namespace is confined to the C++ class. It immediately precedes the method implementations for the class.
- Follow class-style formatting conventions for namespaces, with members and comments lined up in pretty columns.

```cpp
// CFoo methods

namespace Foo
{
    TWEAKABLE bool     s_fDisableFeature;     // Turn off feature
    TWEAKABLE SRgba    s_rgbaEdge;            // Debug drawing
};

CFoo::CFoo()
{
}
```

---

## Function Arguments

Inputs come first, outputs come last, like in a sentence.

```cpp
void ComputeQuadraticRoots(float gC, float gB, float gA, float * pGRoot1, float * pGRoot2);
```

---

## Function Formatting

Functions declarations that are too long for one line should have arguments each go on a new line starting with the first, tabbed in once past the function name. Calls to that function should be one tab in from the function name.

```cpp
// Declaration in .h

bool FTryComputeSomethingBig(
        float gA,
        float gB,
        float gC,
        float gD,
        float gE,
        float gF,
        float * pGOut);

// Class/struct inline definition in .h

float GComputeSomethingSmall(
        float gA,
        float gB)
        {
            float gC = gA * gA;
            return gC + gB;
        }

// Definition in .cpp

bool FTryComputeSomethingBig(
    float gA,
    float gB,
    float gC,
    float gD,
    float gE,
    float gF,
    float * pGOut)
{
    // ...
}

// Call the function

return FTryComputeSomethingBig(
            gArgA,
            gArgB,
            gArgC,
            gArgD,
            gArgE,
            gArgF,
            pGOut);
```

---

## Constructor Formatting

Constructors are formatted similar to functions. In headers the initializer list can be placed on one line. When broken over multiple lines, the colon goes on the same line and all other members are aligned with tabs.

```cpp
// Definition in header

class CClassName2
{
    CClassName2(int a, int b) :
        m_a(a), m_b(b)
        { ; }
};

class CClassName
{
public:
    CClassName() :
        m_nMember1(1),
        m_nMember2(2),
        m_nMember3(3)
        { ; }

    CClassName(
        int n1,
        int n2,
        int n3) :
        m_nMember1(n1),
        m_nMember2(n2),
        m_nMember3(n3)
        { ; }
};

// Definition in .cpp

CClassName::CClassName() :
    m_nMember1(1),
    m_nMember2(2),
    m_nMember3(3)
{
}

CClassName::CClassName(
    int n1,
    int n2,
    int n3) :
    m_nMember1(n1),
    m_nMember2(n2),
    m_nMember3(n3)
{
}

// If an initializer needs to be split on multiple lines, tab in once from the function name:

CClassName::CClassName() :
    super(
        SomeOtherNamespace::s_pos,
        StrFromLit("some really long string")),
    m_nMember1(1),
    m_nMember2(2),
    m_nMember3(3)
{
}

// Dealing with preprocessor directives

CClassName::CClassName() :
    m_nMember1(1),
#if !SHIP
    m_nMember2(2),
#endif
    m_nMember3(3),
    m_nMember4(4)
#if DEBUG
    , m_nMember5(5),
    m_nMember6(6)
#endif
{
}

// Dealing with preprocessor removing all initializers
// (can't have a trailing : with no initializers)

CClassName::CClassName()
#if LATER
    : m_nMember1(1),
    m_nMember2(2)
#endif
{
}
```

---

## Asserts, Errors and Warnings

When in doubt, add error detection and reporting. These are more valuable than any comments, assumptions, or black magic knowledge. They also serve as a great way to document your code.

Use asserts in situations which only other programmers might encounter. For dangerous asserts which will cause big problems later (e.g., memory stomping, about to crash) use `ASSERT_BREAK`. Use errors and warnings in situations which artists/designers and other internal users of the program might encounter. Maintain this philosophy between tools and the engine.

Philosophically, you should not be able to crash the engine without writing C++ code. Error cases from Splice, modeling errors, and so on, should result in a warning or error, followed by graceful handling in the engine.

### Asserts

- Use `ASSERT` for most assertable cases. Make sure the argument has no side effects which would influence behavior.
- Use `CASSERT` for compile-time asserts. Prefer over `ASSERT` where possible.
- The `DASSERT` macro only compiles in the debug build. It is useful for expensive checks and validation, but should be used sparingly. Remember that very few people will be running the debug build.
- Use `VERIFY` when you want the tested expression to run in the ship version. For instance, if you want to assert that the return value from a function has some expected value, but still want to execute the function in the shipping game.
- Use `ASSERT`s at the start of a function to make sure that the parameters passed in are correct.
- If you provide an auditing function on the class which checks it for validity, then it should be named `FIsValid`. If the auditing function does not have a return value, but instead asserts for validity inside the function, then it should be named `Audit`.

### Errors and Warnings

- Use errors and/or warnings to report when an artist or designer does something wrong.
- Gracefully handle error cases, even in the ship build. For example, if a function adds an integer to a fixed-size array, check for overflow and early return.
- Use the error stack to provide useful messages.
- Provide as much information as you can.

---

## Comments

Consider your comments a story describing the system. Class comments are one part of the story, method signature comments are another part, method arguments another part, and method implementation yet another. All these parts should weave together and inform someone else at another point in time just exactly what you did and why. Ideally the code itself will be clear enough to answer most questions, through good use of naming, asserts, data structures, clear breakdowns, and simplified control flow. However, there are numerous places where comments are useful.

### General

- Use `//` comments instead of `/* */` comments.
- Write comments as you code. Definitely write comments before submitting the code changes.
- Comments always have a blank line before and after them. Notable exceptions:
    - End-of-line comments.
    - The newline above the comment is optional if the comment is at the top of a newly-indented block of code.
- Use `BB(username)` to indicate an area that you think needs improving.
- Use `NOTE(username)` to indicate why you made a non-obvious, but generally good choice.
- Descriptive comments for functions aren't required, but they are suggested.
- Don't waste time making beautifully formatted banners.
- Don't worry about Doxygen or other document-generating extractors. Use the code base as documentation. Save a tree.
- All members of structs/classes should be commented, except the trivially obvious ones.

### Header Files

- Provide a one-to-two-line description of the purpose of the class. If you can't make it short, it probably means the class may need to be refactored.
- Provide one-line comments for each non-obvious method or global function.
- Provide end-of-line comments for each non-obvious data member or global data.

### Code Files (Including Inlined Code)

- Don't comment local variables.
- Comments shouldn't just repeat the line of code below it.
- Use comments to explain the block of code which follows. This helps separate straight-line sections, too.
- Provide URLs to places describing the algorithm you are using. Visual Studio will follow URLs in comments.

---

## Spacing

Here are the following rules for spacing:

- 3 newlines between `#include`s and code.
- 3 newlines between different sections of a source file (e.g., class method implementations).
- 1 newline between function implementations.
- 1 newline between related classes (e.g., `SLoData` and `CLo`, related classes in a header).
- 0–1 newlines between code in a function.
- 1 newline between comments and code.

---

## Access Control

Making judicious use of access control on classes helps expose the intended use of the class through the public interface. It also helps limit incorrect usage due to compiler errors.

- In general, place methods in the public interface and data in the protected interface. Some methods may belong in the protected interface.
- Instead of exposing a single method, member data, or accessor function for a single use, try to `friend` that single use. This keeps the public interface small.
- In general, don't be afraid to use the `friend` mechanism. Access control is supposed to be helping you write code, not hurting. If you have a pair, or small set, of classes that work together, it's fine to `friend` them to each other.
- Use accessors when necessary instead of exposing data. These can limit to get-only access, and can check data when setting.
- Don't use `private` access; use `protected` access instead, so derived classes can access base class information.
- Place automatically generated functions in the `protected` section if you don't want clients to use them. This includes the default constructor, the copy constructor, the destructor, and the assignment operator.
- If you really want to prohibit something — say, copy construction — then you can add the method to a `private` section of the class. This should be extremely rare.
- Lay out the class declaration so the `public` section comes first, followed by the `protected` section. Generally, you should have a single `public` section, followed by a single `protected` section.

---

## Constructors and Destructors

In general, follow good behavior in constructors and destructors as described in the books listed at the top. There are a number of gotchas though:

- In the constructors, initialize everything that needs to be initialized. This includes members which have parameterless constructors; they should be explicitly initialized.
- Use virtual destructors where necessary.
- For "bushy" class hierarchies like the `CBasic` tree, use extremely simple constructors and destructors, and do more work in `Init` and `Destroy` methods.
- For redundant constructor code, consider using delegating constructors.
- Use the `explicit` keyword on single-argument constructors to avoid implicit conversions.
- Don't try to handle double destruction.
- Use initializer lists rather than code in the body of the constructor to initialize member variables when possible.
- Initializing arrays of constructor-less types via constructor syntax is accepted practice — however, note that this will zero-fill the array, which may not be advisable.
- Members should be listed in the order in which they appear in the class declaration.
- Don't do virtual dispatch within constructors or destructors. If you need to do virtual dispatch during the construction or destruction process, split out a separate method (like `Init` or `Unload`) and make your virtual function calls there.
- Avoid `memset`, `memcpy`, `memmove` and other bit-clobbering routines unless you know what you're doing.

---

## Const

Strive for `const` correctness. The primary benefit is to document your intent. A secondary benefit is to let the compiler prevent inappropriate use. Although it may seem like the compiler could generate better performing code with `const`, this turns out not to be the case in general (see http://www.gotw.ca/gotw/081.htm).

- Use `const` on read-only class methods.
- Use `const` for pointed-to (or referenced-to) objects.
- Do not use `const` for primitive types or on the pointer/reference itself.
- Avoid `const_cast` as much as possible. Prefer `mutable` where appropriate.
- Don't change semantics if you override a method simply on constness.
- Use `TWEAKABLE` instead of global or static consts for magic numbers.

```cpp
class CFoo
{
public:
    void MutateSelf();          // don't make const
    bool FIsAwesome() const;
};

bool FIsFooAwesome(const CFoo * pFoo)
{
    if (pFoo)
        return pFoo->FIsAwesome();
    else
        return false;
}

void MyFooFunction()
{
    CFoo * pFoo = new CFoo;
    const CFoo * pFooConst = pFoo;  // const on pointed-to object

    // The following line is valid C++, but we don't want to
    // specify const on pointer. Sometimes this is required for sort functions.

    const CFoo * const pFooConstConst = pFoo;

    // The following line is also valid C++, but we don't want
    // to use const on local primitives.
    const bool fAwesome = pFoo->FIsAwesome();

    // Use TWEAKABLE instead of static const.
    static const int s_cIterMax = 20;   // Bad.
    TWEAKABLE int s_cIterMax = 20;      // Good
}

// CConstOverride is a bad example of how overloading on const
// gives very different semantics.
//
// Since we specify return types in the function this typically
// won't show up.

class CConstOverride
{
public:
    void DoSomething() const
        { printf("Happy happy joy joy"); }

    void DoSomething()
        { delete this; }
};
```

---

## Type Casting

Try to avoid type casting. Between function overloads, virtual dispatch, and templates you should be able to eliminate most of it.

In those cases where it is convenient or necessary to cast, use C++ style casting instead of C-style casting. The main benefit is to be clear with your intent.

- Use `reinterpret_cast` where appropriate. Be careful of accessing the same memory between `reinterpret_cast`-ed types. Compilers often optimize load/store code based on types that "cannot alias." Using unions can convince the compiler to perform the loads and stores in the correct order.
- Use `derived_cast` on `CBasic`-derived classes (which use the RTI macro).
- Use `rti_cast` on `CBasic`-derived classes if the class may not be the correct class (combines checking `FIsDerivedFrom` and `derived_cast`).
- Use `static_cast` in other situations.
- Do not use `dynamic_cast`, since we don't support RTTI.
- Try to avoid `const_cast`.
- Avoid implicit type conversions as much as possible.
- For class type conversion, when you need a cast operator prefer using the `explicit` keyword to avoid implicit type conversion.
- When the compiler requires a cast, constructor-style casting is encouraged for simple types (e.g., converting an `int` to an enum or a `Scalar` to a `float`).

```cpp
CLo * PLoFirstChild(CLo * pLoParent)
{
    // If the parent exists, make sure it is an SO.
    if (pLoParent == NULL || !pLoParent->FIsDerivedFrom(CID_SO))
    {
        return NULL;
    }

    CSo * pSoParent = derived_cast<CSo *>(pLoParent);
    return pSoParent->PLoFirstChild();
}

CSo * pSo = new CSo;
const CLo * pLo = pSo;

// bad idea — PLoFirstChild should take a const Lo argument.
CLo * pLoFirstChild = PLoFirstChild(const_cast<CLo *>(pLo));

// Reasonable use of reinterpret_cast
int nOneFpRep = 0x3f800000;
u8 * pBMemN = reinterpret_cast<u8 *>(&nOneFpRep);
ClearAb(pBMemN, sizeof(int));

// Common uses of constructor casting
Scalar g = 1.0f;
TRACE(true, "%.2f\n", float(g));

for (BARK bark = BARK::Min; bark < BARK::Max; ++bark)
{
}
```

---

## Ignoring the Return Value from a Function

If you ignore the return value from a function, make your intent explicit by casting the return value to `void`. This is the sole case where we use the old-style C casting syntax:

```cpp
bool FTrySomething()
{
    return false;
}

void Bar()
{
    (void) FTrySomething();
}
```

---

## Memory Management

Keeping objects alive when they need to be alive is very difficult in C++. Couple this with worrying about memory allocation performance in the engine and you have very tricky code to write.

- Use smart pointers when an object owns the lifetime of another object.
- Make sure you don't create cycles of smart pointers; this will cause memory to never be freed.
- Use weak pointers to `CLo`-derived objects at all times. Never assume that you can control the lifetime of a `CLo`. Use strong pointers for other `CBasic`-derived objects.
- Use the `new`/`delete` family of allocation to create objects.
- Use copy-on-write style semantics where appropriate.
- Add lots of asserts and debugging code to make sure the lifetimes of the objects are as expected.
- Avoid allocation of small objects due to memory overhead. If needed, `CSlotHeap` or similar constructs can be used to speed this up.

---

## Macros (Expanded)

Try to avoid macros. Between inline functions, template functions, enums, and `TWEAKABLE` constants you should be able to avoid most cases.

However, they are useful for conditional compilation and for reducing repeated typing in certain scenarios.

- Don't use macros to `#define` constants.
- Don't use macro functions where a template or inline function would do.
- For conditional compilation, use `#if` instead of `#ifdef`.
- Format macros with `ALL_CAPS_AND_UNDERSCORES`.
- Use `#if DEBUG || NORMAL` instead of `#if DEBUG_NORMAL` to identify builds.
- `#if` logic should have no indentation. Any `#if` region longer than a handful of lines should have a comment specifying what the previous `#if` scope was.
- Define macros used in a lot of different files in the engine `ne_ver.h` to avoid some files receiving the macro value and some not.

---

## The Ternary Operator

Use of the ternary operator (`?:`) is allowed for cases in which the conditional and arguments are very simple. Parentheses are required around the conditional part (before the `?`). If the entire expression doesn't fit on a single line, write it on three lines.

```cpp
float sRange = (pXo->FIsDerivedFrom<CPed>()) ? s_sRangeForPed : s_sRangeDefault;

const CArray<SVerboseDescription *> & arypVerbdsc = (s_fIsMoreVerbosityRequired) ?
                                                        arypVerbdscLong :
                                                        arypVerbdscShort;
```

---

## Booleans

You can use booleans as function arguments. In some cases raw values can be confusing though. Flag enums for multiple options, two-value enums in place of `true`/`false`, or commenting the name of the argument are ways to aid readability.

---

## Magic Numbers

Magic numbers are constant integers, floating points, limits, and vectors sprinkled throughout the code. Try to avoid them, since they make deciphering the code more difficult. They are also harder to change in the debugger.

- Try to restrict magic numbers to `0` or `1` where their usage is obvious.
- Use the `DIM` macro for fixed arrays. Or, just use a `CFixAry`.
- You can use enums for scoped integer constants.
- Use `TWEAKABLE` instead of `const`. This becomes `const` in ship build, but lets you adjust the value in debug and normal builds.
- Any sort of "Nil" value (e.g., in enums) should be defined as `-1`.

---

## Pointers and References

- References are encouraged, but only as an optimization for pass-by-value. This means all references should be `const` references.
- One exception is passing writable arrays to functions. There you should pass non-const `array &` (`float (& aG)[3]`) instead of a C array (`float aG[3]`), since the former is type-safe based on size.
- Use pointers for objects such as `CLo`s, even if they are guaranteed non-NULL and even if they will only have `const` functions called on them, since pass-by-value makes little sense for them.
- You can use `const` references for local variables, assuming construction-and-assignment of a local would also make sense.
- Use by-value passing instead of `const`-ref passing if the values are smaller than 16 bytes and if the copy construction semantics are pretty basic.
- Use `nullptr` in place of `NULL` when initializing pointers.

---

## Local Variables

The key word here is "local." Make the variables have as short a lifetime as possible, which will reduce the amount of data people need to keep in their head when re-reading the code.

- Declare local variables in the most nested scope.
- Try to initialize local variables on the same line that you declare them.
- Only declare one local variable per line.
- Follow the new C++ style of loop scoping.
- Use each local variable for one purpose only. Don't shy away from creating new local variables for different meanings.
- Name local variables according to the rules at the top of the document.
- You can create local scopes to control variable lifetime, but think about creating a new function if this is necessary.

---

## Standard Typedefs

A few standard typedefs make writing the code easier.

- `super` for the direct base class. Use in multiple inheritance if there is a primary superclass, mixed with a bunch of interface definitions.
- `self` inside of template classes, which is its own type.

---

## Formatting

For the most part, the formatting example should be used as the final statement on how code should be formatted. What follows are the main guidelines.

- Comments (almost) always have a blank line before and after them. See the Comments section for details.
- Long lines should be broken once they're past 120 characters.
- Brackets for static array initialization should not be tabbed in.

```cpp
static SVertexFormat s_aVtxfmtSkin[] =
{
    { 3, GL_FLOAT, GLUSAGE_Position, 0, offsetof(SVtx, m_pos) },
    { 3, GL_FLOAT, GLUSAGE_Normal,   0, offsetof(SVtx, m_normal) },
};
```

---

## Data Alignment

Use the `ALIGN(n)` macro to align data and structures. To ensure correctness across platforms, follow these rules:

For aligned structures & classes, put `ALIGN(n)` just after the `struct` or `class` keyword.

```cpp
// INCORRECT
struct SFoo
{
    u8 m_aB[8];
} ALIGN(16);

// CORRECT
struct ALIGN(16) SFoo
{
    u8 m_aB[8];
};
```

For variables, put `ALIGN(n)` before the type specification.

```cpp
// INCORRECT
u8 g_aB[8] ALIGN(16);
u8 ALIGN(16) g_aB[8];

// CORRECT
ALIGN(16) u8 g_aB[8];
```

These rules are difficult to enforce via macros and asserts, so keep an eye out for them.

---

## Files

- Files should be descriptively named, often with the name of the primary class or system in a file.
- Files in AC with the same name as class files in NE should be named prefixed with `m`.

---

## CString

`CString` is our ref-counted, immutable string type. It treats strings as immutable and stores the refcount and bytes in a single allocation. The characters are normally UTF-8.

Guidelines for using `CString`:

- `CString` usage is fine in tools and in many contexts in development builds.
- For shipping engine code, prefer OIDs to `CString` where possible. Using strings is often much slower than using OIDs.
- In Splice, `CString` support is relatively poor outside of dev builds. This is partly to dissuade using them at runtime when unnecessary.
- Use `StrFromLit` to make literal `CString`s.
- For `SData` classes you can dump `CDataString` or `CDebugString` using `WriteString` or `WriteDebugString` on `CBinaryOutputStream` in AC. Take care storing these elsewhere in the engine though as the data can be unloaded.

---

## C++11 (and Later) Features

C++11 (and later) features are only allowed in the following cases:

- `override` — use in place of `virtual` for all overridden virtual functions.
- `final` — may be used where appropriate.
- Range-based `for` is allowed for all usage.
- `auto` is only allowed for range-based `for` with `const auto &` (or with `auto *` or similar) and, rarely, for template iterator types (see STL disallowed below). Consider readability when using `auto`.
- `default` and `delete` class specifiers are allowed. Use `delete` instead of private unimplemented functions.
- `initializer_list`s are allowed for statically scoped lists.
- Lambdas can be used in place of functors (e.g., as an argument to a sort function). Don't use default capture syntax (`[&]` and `[=]`) unless it's in debug code and performance is not a consideration. Consider readability when using lambdas to compress duplicate code involving state.
- Move constructors are allowed. Generally try to avoid returning collections or things with expensive constructors by value.
- Variadic templates may be used where appropriate.
- Raw string literals may be used where appropriate.
- `decltype` is allowed for use. As this is generally a niche template use, take care the code is not being over-complicated.
- Class member default values in place of simple constructors. If any values are complex or need to be tuned, use a constructor instead.
- Brace (aggregate) initialization is allowed only for function arguments and return values. For value initialization use constructor syntax with parentheses.
- For constant definition, prefer `static const` to `constexpr` where equivalent unless `constexpr` is required (e.g., `CASSERT`s).
- Take care to avoid compile-time overhead for commonly included header files (e.g., `constexpr` but also template instantiation).

---

## Feature Support

This is a quick list of C++ features not covered in the rest of the document, broken into which are allowed, which are frowned upon, and which are disallowed.

### Allowed

- Templates, within reason
- Operator overloading (follow the syntax and semantic conventions)
- Nested classes
- Namespaces
- Multiple inheritance with interfaces
- Anonymous unions

### Frowned Upon

- Varargs
- Crazy template metaprogramming (including most Boost code)
- Pimpl idiom

### Disallowed

- Exceptions
- RTTI
- STL
- Iostream

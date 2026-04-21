# Lisp Language Specification

Lisp is a tiny Lisp dialect that compiles to LLVM IR via direct LLVM assembly generation.

## Version
2.0.0

## File Extension
`.lisp`

## Compiler Usage
```bash
./lisp input.lisp              # compile and execute
./lisp input.lisp -o output     # compile to executable
./lisp input.lisp --emit-ir     # output .ll file
./lisp input.lisp --emit-asm    # output .s file
./lisp input.lisp --emit-obj    # output .o file
./lisp input.lisp --verbose     # preserve intermediate files
./lisp input.lisp --version     # show version
```

---

## Lexical Structure

### Comments
```
; everything after ; until end of line is a comment
```

### Whitespace
Spaces, tabs, and newlines are ignored as whitespace.

### Identifiers
Letters, digits, underscores, hyphens, and question marks.
```
abc  foo-bar  hello_world  test?123
```

### Special Characters in Tokens
- `( )` — parentheses for S-expressions
- `[ ]` — brackets for cond clauses
- `;` — comment start
- `#\` — character literal prefix
- `" "` — string literal delimiters

---

## Data Types

### Integer (`int`)
- 32-bit signed integer
- Literals: `42`, `-5`, `0`
- Operations: `+`, `-`, `*`, `/`, `%`

### Float (`float`)
- 64-bit double precision
- Literals: `3.14`, `-0.5`, `2.0`
- Operations: `+`, `-`, `*`, `/` (float variants)

### Character (`char`)
- 8-bit integer representing a character
- Literals: `#\a`, `#\newline`, `#\space`, `#\tab`, `#\null`
- Single character after `#\`

### String (`ptr`)
- Null-terminated C string (`i8*`)
- Literals: `"hello world"`
- Printed with `puts`

### Boolean (`bool`)
- 1-bit integer (i1)
- Literals: `#t` (true = 1), `#f` (false = 0)

---

## Keywords (Special Forms)

### `(def name value)` — Variable Definition
```lisp
(def x 10)
(def name "Alice")
(def pi 3.14159)
```

### `(def (fname param...) body...)` — Function Definition
```lisp
(def (add a b)
  (+ a b))
(add 3 4)  ; => 7
```
- Return type is inferred
- Optional explicit return type: `(def (fname : ret-type param...) body...)`

### `(if cond then else)` — Conditional
```lisp
(if #t 100 200)     ; => 100
(if (< x 0) 0 x)
```
- Condition is any expression
- Then/else are expressions

### `(cond [cond1 expr1] [cond2 expr2] ...)` — Conditional Chain
```lisp
(cond
  [(< x 0) "negative"]
  [(= x 0) "zero"]
  [#t "positive"])
```
- Evaluates conditions sequentially
- Returns value of first truthy condition's expression

### `(loop n expr...)` — Simple Loop
```lisp
(loop 5 (+ i 1))    ; executes body 5 times, returns last value
```
- Executes body expressions N times
- Returns the last expression's value

### `(while cond body...)` — While Loop
```lisp
(def i 0)
(while (< i 5)
  (def i (+ i 1)))
i  ; => 5
```
- Evaluates condition, if true executes body and repeats

### `(for var init cond step body...)` — For Loop
```lisp
(for i 0 (< i 5) (+ i 1)
  (body))
```
- var: loop variable name
- init: initial value expression
- cond: condition expression
- step: step expression (evaluated after each iteration)

### `(mac name (param...) body...)` — Macro Definition
```lisp
(mac twice (x)
  (+ x x))
(twice 5)   ; expands to (+ 5 5) => 10
```
- Macros perform textual substitution before evaluation
- Parameters are replaced literally in the body

### `(import "path")` — Import
```lisp
(import "utils.lisp")
```
- Loads and evaluates another source file
- Relative paths are relative to current file's directory

### `(ffi ret-type name (param-types...))` — Foreign Function Interface
```lisp
(ffi int putchar (int))
(ffi int getchar ())
```
- Declares an external C function
- FFI types: `int`, `float`, `char`, `ptr`

### `(llvm "ir code")` — Raw LLVM IR Injection
```lisp
(llvm "%result = add i32 1, 2")
```
- Emits raw LLVM IR into the output
- Must be valid LLVM IR syntax

---

## Operators

### Arithmetic
| Operator | Integer Op | Float Op | Description |
|----------|-----------|----------|-------------|
| `+` | `add` | `fadd` | Addition |
| `-` | `sub` | `fsub` | Subtraction |
| `*` | `mul` | `fmul` | Multiplication |
| `/` | `sdiv` | `fdiv` | Division |
| `%` | `srem` | `frem` | Modulo |

### Comparison
| Operator | Integer Op | Float Op | Description |
|----------|-----------|----------|-------------|
| `<` | `slt` | `olt` | Less than |
| `>` | `sgt` | `ogt` | Greater than |
| `<=` | `sle` | `ole` | Less or equal |
| `>=` | `sge` | `oge` | Greater or equal |
| `=` | `eq` | `oeq` | Equality |
| `!=` | `ne` | `one` | Not equal |

---

## Built-in Functions

These functions are pre-declared and don't need FFI:

### `(puts str)` — Print String
```lisp
(puts "hello")    ; prints "hello" + newline
```

### `(printf fmt arg...)` — Formatted Print
```lisp
(printf "%d" 42)          ; prints "42"
(printf "%f" 3.14)        ; prints "3.140000"
```
Format specifiers: `%d`, `%f`, `%c`, `%s`

### `(putchar int-or-char)` — Print Character
```lisp
(putchar 65)    ; prints 'A'
(putchar #\a)   ; prints 'a'
```

---

## FFI Type Reference

When declaring external functions with `ffi`:

| Lisp Type | LLVM Type | Description |
|-----------|-----------|-------------|
| `int` | `i32` | 32-bit integer |
| `float` | `double` | 64-bit float |
| `char` | `i8` | 8-bit character |
| `ptr` | `i8*` | Pointer type |
| `.` (dot) | `...` (variadic) | Variadic argument |

Example with variadic:
```lisp
(ffi int printf (ptr .))
```

---

## Evaluation Model

1. **Load phase**: Read all files via `import`
2. **Macro expansion**: Expand all macros before evaluation
3. **Evaluation**: Top-level expressions evaluate in order
4. **Output**: Last expression's value is printed (or discarded if void)

---

## Type Inference Rules

The compiler infers types automatically:

- Integer literals → `int`
- Float literals → `float`
- Character literals → `char`
- String literals → `ptr` (i8*)
- Boolean literals → `bool` (i1)
- Binary operations → type of operands (float if either operand is float)
- Function return → type of last expression

### Explicit Return Type Annotation
```lisp
(def (get-char-code : int c)
  c)
```
The `: ret-type` syntax allows explicit return type specification.

---

## LLVM IR Output

When using `--emit-ir`, the generated .ll file includes:

1. Standard library declarations (`printf`, `puts`, `putchar`)
2. Format strings for output functions
3. The compiled `main` function
4. Any raw LLVM IR injected via `(llvm ...)`

---

## Error Handling

| Error | Cause |
|-------|-------|
| `expected operator` | Syntax error, mismatched parentheses |
| `llc: error` | Invalid LLVM IR generated |
| `cannot open import` | Import file not found |
| `expected )` | Missing closing parenthesis |
| `expected identifier` | Missing identifier after keyword |

---

## Design Notes

- **No tail-call optimization** — loops are the preferred recursion alternative
- **Variables are mutable** — `(def x ...)` reassigns variables
- **Strings are i8*** — null-terminated C strings, compatible with C libraries
- **Macros are hygienic** — textual substitution, no variable capture
- **Static typing** — types inferred, but no runtime type errors (undefined behavior instead)

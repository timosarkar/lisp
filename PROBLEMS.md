# Known Problems and TODOs

## Compiler Bugs

### 1. Recursive calls inside `if` expressions fail
**Severity:** High
**Description:** When a function calls itself recursively inside an `if` branch, the compiler fails with "unexpected token". This is because nested functions aren't added to the `funcs` map until after the function body is parsed.

**Example that fails:**
```lisp
(def (lp i r)
  (if (= i 0)
    r
    (lp (- i 1) (* r i))))  ; Recursive call fails
```

**Workaround:** Define helper functions before use, but recursive calls still don't work inside `if` bodies.

### 2. `printf` with numeric arguments produces wrong output
**Severity:** High
**Description:** When calling `printf` with a numeric argument (like `printf "%d" 12345`), the output is garbage (-18426725610). This appears to be a register naming issue where nested calls share the same temporary register.

**Example:**
```lisp
(printf "%d" 12345)  ; Outputs garbage instead of 12345
```

**Works:**
```lisp
(puts "hello")  ; Works correctly
```

### 3. `?` in function names not supported
**Severity:** Low
**Description:** LLVM doesn't allow `?` characters in function names, but the lisp dialect used predicates like `zero?`, `pos?`, etc.

**Workaround:** Use `_p` suffix instead (e.g., `zero_p`, `pos_p`).

### 4. `not` keyword not recognized
**Severity:** Low
**Description:** `not` is treated as a normal identifier, not a keyword. Must use `(if x 0 1)` instead.

**Workaround:**
```lisp
(def (not x) (if x 0 1))
```

## Missing Features

### 5. String concatenation doesn't work
**Severity:** Medium
**Description:** String concatenation using `+` doesn't work when nested inside function calls.

**Example that fails:**
```lisp
(def (greet name) (puts (+ "Hello, " name)))
```

### 6. No arrays/vectors
**Severity:** Medium
**Description:** The language has no support for arrays or collections. This limits what algorithms can be implemented.

**Workaround:** Use recursion with immutable values (not practical for large datasets).

### 7. No structs/objects
**Severity:** Medium
**Description:** No way to define composite data types.

### 8. No module system beyond `import`
**Severity:** Low
**Description:** Only basic file inclusion via `(import "file.lisp")`. No namespacing or module exports.

## Performance Issues

### 9. C benchmarks have int overflow
**Severity:** Low (in benchmarks only)
**Description:** The fibonacci C benchmark uses `int` instead of `long long`, causing overflow (1599621376 instead of 83204000000).

**Note:** This is only in the benchmark code, not the compiler itself.

## Documentation

### 10. Tutorial could be expanded
**Severity:** Low
**Description:** The tutorial.txt could include more examples and explain advanced features better.

### 11. Missing FFI documentation
**Severity:** Low
**Description:** How to properly use `(ffi ret-type name (param-types...))` could be documented better.

## Code Quality

### 12. Error messages could be more helpful
**Severity:** Medium
**Description:** Some error messages like "unexpected token" don't always point to the actual issue.

### 13. No AST visualization or debugging
**Severity:** Low
**Description:** No way to see the parsed AST or intermediate representation for debugging.

## Cleanup Needed

- [ ] Remove temporary test files (test_simple.lisp, test_printf.lisp, etc.)
- [ ] Consider consolidating some test files
- [ ] Add more documentation on language limitations

## Priority Order

1. **Fix printf bug** - Currently breaks basic numeric output
2. **Fix recursive calls in if** - Breaks many algorithms
3. **Add string concatenation** - Common need
4. **Add basic arrays** - Enables more algorithms
5. **Clean up documentation** - Helps users

---

*Last updated: 2026-04-21*
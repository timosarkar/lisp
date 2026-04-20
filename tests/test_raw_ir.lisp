; LLVM IR - just emit some llvm ir without function call
; This just tests that llvm syntax works
(llvm "%x = add i32 1, 2")
(+ 1 2)
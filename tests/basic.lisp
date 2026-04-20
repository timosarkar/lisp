; Basic arithmetic
(def a 10)
(def b 20)
(+ a b)

; Float arithmetic
(def f 3.14)
(* f 2.0)

; String
(def s "hello")
s

; Boolean
(def b1 #t)
(def b2 #f)
b1

; Comparison
(< 5 10)

; If expression
(if #t 1 2)

; Variable shadowing and scopes
(def x 5)
(def y 10)
(+ x y)

; Multiple operations
(def n 100)
(+ n (* n 2))

; Character
(def c #\a)
c

; Nested expressions
(* (+ 2 3) (+ 4 5))

; Cond expression
(cond [#t 42] [#f 99])
; benchmark - simple iterative
(def (fact n)
  (def (lp i r)
    (if (= i 0)
      r
      (lp (- i 1) (* r i))))
  (lp n 1))

(print-int (fact 12))
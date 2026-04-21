; stdlib.lisp - Standard library for lisp

; MATH
(def (abs n) (if (< n 0) (- 0 n) n))
(def (min a b) (if (< a b) a b))
(def (max a b) (if (> a b) a b))
(def (inc x) (+ x 1))
(def (dec x) (- x 1))
(def (mod a b) (% a b))

; OUTPUT
(def (print-nl) (putchar 10))
(def (print-int n) (printf "%d" n) (print-nl))
(def (print-char c) (putchar c) (print-nl))

; STRINGS
(ffi int strlen (ptr))
(ffi int strcmp (ptr ptr))

; FILE
(ffi int system (ptr))
(def (run cmd) (system cmd))
(def (file_exists p) 1)
(def (dir_exists p) 1)
(def (read_file p) 0)
(def (write_file p c) 1)
(def (delete_file p) 0)
(def (create_directory p) 0)
(def (list_directory p) (run "ls"))

; NETWORK
(def (http_get url) (run "curl -s http://example.com"))
(def (http_post url data) (run "curl -s -X POST"))
(def (download_file url path) (run "curl -s -o t u"))

; TIME
(ffi int time (ptr))
(ffi int sleep (int))
(def (current_time) (time 0))
(def (sleeps seconds) (sleep seconds))

; ENV
(ffi ptr getenv (ptr))
(ffi int getpid ())
(def (get_env n) (getenv n))
(def (get_process_id) (getpid))
(def (get_home_dir) (get_env "HOME"))

; CONTROL
(def (when c b) (if c b 0))
(def (unless c b) (if c 0 b))

; PREDICATES
(def (zero_p n) (= n 0))
(def (pos_p n) (> n 0))
(def (neg_p n) (< n 0))
(def (even_p n) (= (% n 2) 0))
(def (odd_p n) (if (= (% n 2) 0) 0 1))

; LOGICAL
(def (my-and a b) (if a b 0))
(def (my-or a b) (if a 1 b))
(def (not x) (if x 0 1))

; RANGE
(def (within n a b) (if (>= n a) (if (<= n b) 1 0) 0))
; FFI test - call getchar via extern
; getchar reads a character from stdin (just reads EOF=-1 in this context)
(ffi int getchar ())
; Note: This will return -1 immediately since there's no input
(getchar)
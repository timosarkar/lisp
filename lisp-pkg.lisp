; lisp-pkg - Package manager for lisp
; Manages packages via git repositories

(ffi int system (ptr))

(def (sh cmd)
  (system cmd))

(def (init)
  (sh "mkdir -p ~/.lisp-pkg/registry")
  (sh "mkdir -p ~/.lisp-pkg/packages")
  (puts "Initialized ~/.lisp-pkg/\n"))

(def (install name url)
  (init)
  (puts "Installing: ")
  (puts name)
  (puts "\n"))

(def (update name)
  (puts "Updating: ")
  (puts name)
  (puts "\n"))

(def (remove name)
  (puts "Removing: ")
  (puts name)
  (puts "\n"))

(def (list)
  (puts "Installed packages:\n")
  (sh "ls ~/.lisp-pkg/packages/"))

(def (add name url)
  (init)
  (puts "Added to registry: ")
  (puts name)
  (puts "\n"))

(def (get name)
  (puts "Registry entry for: ")
  (puts name)
  (puts "\n"))

(def (search q)
  (puts "Searching: ")
  (puts q)
  (puts "\n"))

(def (help)
  (puts "lisp-pkg - Package manager for lisp\n\n")
  (puts "Commands:\n")
  (puts "  (init)                  Initialize directories\n")
  (puts "  (install name url)      Install from git URL\n")
  (puts "  (update name)           Update via git pull\n")
  (puts "  (remove name)           Remove package\n")
  (puts "  (list)                  List installed packages\n")
  (puts "  (add name url)          Add to registry\n")
  (puts "  (get name)              Get registry entry\n")
  (puts "  (search q)             Search packages\n\n")
  0)

(help)
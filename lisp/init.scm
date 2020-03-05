(use-modules (system repl server))

(spawn-server (make-tcp-server-socket #:port 4005))

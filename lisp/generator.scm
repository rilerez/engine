(define-module (generators)
  #:export (yield generator))

(define-syntax-parameter yield
  (lambda (stx)
    (syntax-violation 'yield "~yield~ is undefined  outside of a generator" stx)))

(define-syntax-rule (generator body ...)
  (let ()
    (define yield-tag (make-prompt-tag))
    (define (yield% . returns)
      (apply abort-to-prompt yield-tag returns))
    (define (thunk)
      (syntax-parameterize ((yield (identifier-syntax yield%)))
        body ...))
    (define this-step thunk)
    (lambda ()
      (call-with-prompt yield-tag
        this-step
        (lambda (next-step . args)
          (set! this-step next-step)
          (apply values args))))))

;; (define count
;;   (generator
;;    (let loop ((i 0))
;;      (yield i)
;;      (loop (1+ i)))))

;; (count)

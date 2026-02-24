(((c-mode c++-mode) . ((eval . (let* ((local-root (project-root (project-current)))
                                      (proj-name (project-name (project-current)))
                                      (remote-src-root (format "/root/remote/%s/" proj-name))
                                      (remote-build-root (format "/root/remote/.build/%s/" proj-name)))
                                 (setf miruku/lsp-path-mapping
                                       (list (cons local-root remote-src-root)
                                             (cons "/sshx:deepin-vm:/" "/"))))))))

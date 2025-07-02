(define-module (quickshell)
  #:use-module ((guix licenses) #:prefix license:)
  #:use-module (gnu packages cpp)
  #:use-module (gnu packages freedesktop)
  #:use-module (gnu packages gcc)
  #:use-module (gnu packages gl)
  #:use-module (gnu packages jemalloc)
  #:use-module (gnu packages linux)
  #:use-module (gnu packages ninja)
  #:use-module (gnu packages pkg-config)
  #:use-module (gnu packages qt)
  #:use-module (gnu packages vulkan)
  #:use-module (gnu packages xdisorg)
  #:use-module (gnu packages xorg)
  #:use-module (guix build-system cmake)
  #:use-module (guix download)
  #:use-module (guix gexp)
  #:use-module (guix git-download)
  #:use-module (guix packages)
  #:use-module (guix packages)
  #:use-module (guix utils))

(define-public quickshell-git
  (package
    (name "quickshell")
    (version "git")
    (source (local-file "." "quickshell-checkout"
                        #:recursive? #t
                        #:select? (or (git-predicate (current-source-directory))
                                      (const #t))))
    (build-system cmake-build-system)
    (propagated-inputs (list qtbase qtdeclarative qtsvg))
    (native-inputs (list ninja
                         gcc-14
                         pkg-config
                         qtshadertools
                         spirv-tools
                         wayland-protocols
                         cli11))
    (inputs (list jemalloc
                  libdrm
                  libxcb
                  libxkbcommon
                  linux-pam
                  mesa
                  pipewire
                  qtbase
                  qtdeclarative
                  qtwayland
                  vulkan-headers
                  wayland))
    (arguments
     (list #:tests? #f
           #:configure-flags
           #~(list "-GNinja"
                   "-DDISTRIBUTOR=\"In-tree Guix channel\""
                   "-DDISTRIBUTOR_DEBUGINFO_AVAILABLE=NO"
                   ;; Breakpad is not currently packaged for Guix.
                   "-DCRASH_REPORTER=OFF")
           #:phases
           #~(modify-phases %standard-phases
               (replace 'build (lambda _ (invoke "cmake" "--build" ".")))
               (replace 'install (lambda _ (invoke "cmake" "--install" ".")))
               (add-after 'install 'wrap-program
                 (lambda* (#:key inputs #:allow-other-keys)
                   (wrap-program (string-append #$output "/bin/quickshell")
                     `("QML_IMPORT_PATH" ":"
                       = (,(getenv "QML_IMPORT_PATH")))))))))
    (home-page "https://quickshell.outfoxxed.me")
    (synopsis "QtQuick-based desktop shell toolkit")
    (description
     "Quickshell is a flexible QtQuick-based toolkit for creating and
customizing toolbars, notification centers, and other desktop
environment tools in a live programming environment.")
    (license license:lgpl3)))

quickshell-git

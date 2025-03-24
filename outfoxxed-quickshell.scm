(define-module (outfoxxed-quickshell)
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
                         pkg-config
                         qtshadertools
                         spirv-tools
			 wayland-protocols
                         gcc-14))
    (inputs (list cli11
                  jemalloc
                  libdrm
                  libxcb
                  libxkbcommon
                  linux-pam
                  mesa
                  pipewire
                  qtdeclarative
                  qtwayland
                  vulkan-headers
                  wayland
                  qtbase))
    (arguments
     (list #:tests? #f
           #:configure-flags #~(list "-GNinja"
                                     "-DDISTRIBUTOR=\"GNU Guix\""
                                     "-DDISTRIBUTOR_DEBUGINFO_AVAILABLE=NO"
                                     "-DCRASH_REPORTER=OFF") ;no breakpad
           #:phases #~(modify-phases %standard-phases
                        (replace 'build
                          (lambda* (#:key parallel-build? #:allow-other-keys)
                            (invoke "cmake" "--build" ".")))
                        (replace 'install
                          (lambda _ (invoke "cmake" "--install" "."))))))
    (home-page "https://git.outfoxxed.me/quickshell/quickshell")
    (synopsis "QtQuick-based desktop shell toolkit")
    (description
     "Quickshell is a flexible QtQuick-based toolkit for creating and
customizing toolbars, notification centers, and other desktop
environment tools in a live programming environment.")
    (license license:lgpl3)))

quickshell-git

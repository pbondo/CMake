include(RunCMake)

run_cmake(DOWNLOAD-hash-mismatch)
run_cmake(DOWNLOAD-unused-argument)
run_cmake(DOWNLOAD-httpheader-not-set)
run_cmake(DOWNLOAD-pass-not-set)
run_cmake(UPLOAD-unused-argument)
run_cmake(UPLOAD-httpheader-not-set)
run_cmake(UPLOAD-pass-not-set)
run_cmake(INSTALL-DIRECTORY)
run_cmake(INSTALL-MESSAGE-bad)
run_cmake(FileOpenFailRead)
run_cmake(LOCK)
run_cmake(LOCK-error-file-create-fail)
run_cmake(LOCK-error-guard-incorrect)
run_cmake(LOCK-error-incorrect-timeout)
run_cmake(LOCK-error-incorrect-timeout-trail)
run_cmake(LOCK-error-lock-fail)
run_cmake(LOCK-error-negative-timeout)
run_cmake(LOCK-error-no-function)
run_cmake(LOCK-error-no-guard)
run_cmake(LOCK-error-no-path)
run_cmake(LOCK-error-no-result-variable)
run_cmake(LOCK-error-no-timeout)
run_cmake(LOCK-error-timeout)
run_cmake(LOCK-error-unknown-option)
run_cmake(LOCK-lowercase)
run_cmake(GLOB)
run_cmake(GLOB_RECURSE)
# test is valid both for GLOB and GLOB_RECURSE
run_cmake(GLOB-error-LIST_DIRECTORIES-not-boolean)
# test is valid both for GLOB and GLOB_RECURSE
run_cmake(GLOB-error-LIST_DIRECTORIES-no-arg)
run_cmake(GLOB-noexp-LIST_DIRECTORIES)

if(NOT WIN32 OR CYGWIN)
  run_cmake(GLOB_RECURSE-cyclic-recursion)
endif()

#
# libbinrec: a recompiling translator for machine code
# Copyright (c) 2016 Andrew Church <achurch@achurch.org>
#
# This software may be copied and redistributed under certain conditions;
# see the file "COPYING" in the source code distribution for details.
# NO WARRANTY is provided with this software.
#

#
# By default, all commands will be displayed in an abbreviated form, such
# as "Compiling foo.c", instead of the actual command lines executed.
# Pass V=1 on the make command line to display the actual commands instead.
#

###########################################################################
############################## Configuration ##############################
###########################################################################

# In addition to the options below, standard build environment variables
# (CC, CFLAGS, etc.) are also recognized.

#----------------------------- Build options -----------------------------#

# BUILD_SHARED:  If this variable is set to 1, the build process will
# create a shared library (typically "libbinrec.so" on Unix-like systems).
# If the variable is set to 0, no shared library will not be built.
#
# The default is 1 (a shared library will be built).

BUILD_SHARED = 1


# BUILD_STATIC:  If this variable is set to 1, the build process will
# create a static library (typically "libbinrec.a" on Unix-like systems).
# If the variable is set to 0, no static library will not be built.  Note
# that tests are linked against the static library, so "make test" will
# cause the static library to be built even if this variable is set to 0.
#
# It is possible, though mostly meaningless, to set both BUILD_SHARED and
# BUILD_STATIC to 0.  In this case, "make" will do nothing, and "make
# install" will install only the library header file.
#
# The default is 1 (a static library will be built).

BUILD_STATIC = 1


# ENABLE_ASSERT:  If this variable is set to 1, assertion checks will be
# compiled into the code to guard against bugs in the library.  This
# requires support for the assert() macro and <assert.h> header in the
# system's runtime libraries.  Enabling this option may have a moderate
# performance impact.
#
# The default is 1 (assertion checks will be included).

ENABLE_ASSERT = 1


# ENABLE_OPERAND_SANITY_CHECKS:  If this variable is set to 1, assertion
# checks will be added on instruction operands to verify that registers
# are being used correctly.  Enabling this option may have a moderate
# performance impact.
#
# The checks covered by this option differ from those covered by
# ENABLE_ASSERT in that assertion failures generally indicate logic bugs
# in the core library code which might otherwise cause a crash, while
# operand check failures generally indicate bugs in specific machine code
# translation modules which might lead to incorrect code being generated
# but are not likely to cause a crash by themselves.
#
# The default is 1 (operand sanity checks will be included).

ENABLE_OPERAND_SANITY_CHECKS = 1


# INSTALL_PKGCONFIG:  If this variable is set to 1, the build process will
# install a control file for the "pkg-config" tool as
# "$(LIBDIR)/pkgconfig/binrec.pc".
#
# The default is 0 (binrec.pc will not be installed).

INSTALL_PKGCONFIG = 0


# STRIP_TESTS:  If this variable is set to 1, test programs will be built
# with debugging information removed.  This makes it harder to debug the
# programs in the debugger, but it can provide a significant savings in
# disk space consumed by the tests.
#
# The default is 1 (test programs will be stripped).

STRIP_TESTS = 1


# TESTS:  If this variable is not empty, it gives a list of tests to be
# executed for "make test" or "make coverage".  Tests should be specified
# by the pathname to the test source file without the ".c" extension; for
# example, "tests/api/log" to run the test defined in tests/api/log.c.
# The default is empty (all tests will be executed).

TESTS =


# TESTS_EXCLUDE:  If this variable is not empty, it gives a list of tests
# to be skipped for "make test" or "make coverage".  Tests should be
# specified as for the TESTS variable.  If both this variable and TESTS
# are non-empty, only tests which are both listed in TESTS and not listed
# in this variable will be executed.  The default is empty (no tests will
# be skipped).

TESTS_EXCLUDE =


# WARNINGS_AS_ERRORS:  If this variable is set to 1, the build will abort
# if the compiler emits any warnings.
#
# The default is 1 (warnings will abort the build).

WARNINGS_AS_ERRORS = 1

#----------------------- Installation target paths -----------------------#

# DESTDIR:  Sets the location of the installation root directory.  This
# string, if any, is prefixed to all target pathnames during installation,
# but it is not included in pathnames written to installed files, allowing
# the package to be installed to a non-root directory hierarchy.
#
# The default is the empty string.

DESTDIR =


# INCDIR:  Sets the directory into which the library's header files
# (binrec.h and binrec++.h) will be installed.
#
# The default is "$(PREFIX)/include".

INCDIR = $(PREFIX)/include


# LIBDIR:  Sets the directory into which the shared and static library
# files will be installed.
#
# The default is "$(PREFIX)/lib".

LIBDIR = $(PREFIX)/lib


# PREFIX:  Sets the base directory for installation paths.  This is only
# used to set the default paths for BINDIR, INCDIR, and LIBDIR, and to set
# the "prefix" tag in the pkg-config control file if it is installed.
#
# The default is "/usr/local".

PREFIX = /usr/local

###########################################################################
############################## Internal data ##############################
###########################################################################

#-------------------------------------------------------------------------#
# Note: You should never need to modify any variables below this point in #
# the file.                                                               #
#-------------------------------------------------------------------------#

# Package name:
PACKAGE = binrec

# Library version:
VERSION = 0.1
VERSION_MAJOR = $(firstword $(subst ., ,$(VERSION)))

# Library output filenames: (note that $(OSTYPE) is set below)
SHARED_LIB = lib$(PACKAGE).$(if $(filter darwin%,$(OSTYPE)),dylib,$(if $(filter mingw%,$(ARCH)),dll,so))
STATIC_LIB = lib$(PACKAGE).a

# Test source list, with optional overriding.
TEST_SOURCES_FULL := $(sort $(wildcard tests/*/*.c tests/*/*/*.c))
TEST_SOURCES := $(TEST_SOURCES_FULL)
ifneq ($(TESTS),)
    TESTS := $(foreach i,$(TESTS),$(or $(filter $i.c,$(TEST_SOURCES_FULL)),$(error Test $i not found for TESTS)))
    TEST_SOURCES := $(TESTS:%=%.c)
endif
ifneq ($(TESTS_EXCLUDE),)
    TESTS_EXCLUDE := $(foreach i,$(TESTS_EXCLUDE),$(or $(filter $i.c,$(TEST_SOURCES_FULL)),$(error Test $i not found for TESTS_EXCLUDE)))
    TEST_SOURCES := $(filter-out $(TESTS_EXCLUDE:%=%.c),$(TEST_SOURCES))
endif

# Source and object filenames:
LIBRARY_SOURCES := $(sort $(wildcard src/*.c src/*/*.c))
LIBRARY_OBJECTS := $(LIBRARY_SOURCES:%.c=%.o)
TEST_OBJECTS := $(TEST_SOURCES:%.c=%.o)
TEST_BINS := $(TEST_SOURCES:%.c=%)
BENCHMARK_SOURCES := $(sort $(wildcard benchmarks/*.c benchmarks/blobs/*.c))
BENCHMARK_OBJECTS := $(BENCHMARK_SOURCES:%.c=%.o) \
                     tests/execute.o tests/log-capture.o \
                     $(patsubst %.c,%_noopt.o,$(wildcard benchmarks/dhrystone/*.c)) \
                     $(patsubst %.c,%_opt.o,$(wildcard benchmarks/dhrystone/*.c))
BENCHMARK_BINS := benchmarks/bench

###########################################################################
#################### Helper functions and definitions #####################
###########################################################################

# if-true:  Return the second parameter if the variable named by the first
# parameter has the value 1, the third parameter (which may be omitted)
# otherwise.

if-true = $(if $(filter 1,$($1)),$2,$3)


# define-if-true:  Return an appropriate -D compiler option for the given
# variable name if its value is 1, the empty string otherwise.

define-if-true = $(call if-true,$1,-D$1)


# ECHO:  Expands to "@:" (a no-op) in verbose (V=1) mode, "@echo" in
# non-verbose mode.  Used for displaying abbreviated command text.

ECHO = $(call if-true,V,@:,@echo)


# Q:  Expands to the empty string in verbose (V=1) mode, "@" otherwise.
# Used to hide output of command lines in non-verbose mode.

Q = $(call if-true,V,,@)

###########################################################################
############################# Toolchain setup #############################
###########################################################################

# Default tool program names:

CC = cc
AR = ar
RANLIB = ranlib
GCOV = $(error gcov program unknown for this compiler)


# Try and guess what sort of compiler we're using, so we can set
# appropriate default options.

CC_TYPE = unknown
ifneq ($(filter clang%,$(subst -, ,$(CC))),)
    CC_TYPE = clang
else ifneq ($(filter icc%,$(subst -, ,$(CC))),)
    CC_TYPE = icc
else ifneq ($(filter gcc%,$(subst -, ,$(CC))),)
    CC_TYPE = gcc
else
    CC_VERSION_TEXT := $(shell '$(CC)' --version 2>&1)
    ifneq (,$(filter clang LLVM,$(CC_VERSION_TEXT)))
        CC_TYPE = clang
    else ifneq (,$(filter gcc,$(CC_VERSION_TEXT)))
        CC_TYPE = gcc
    endif
endif

ifeq ($(CC_TYPE),clang)
    BASE_FLAGS = -O2 -pipe -g -I. \
        $(call if-true,WARNINGS_AS_ERRORS,-Werror) \
        -Wall -Wextra -Wcast-align -Winit-self -Wpointer-arith -Wshadow \
        -Wwrite-strings -Wundef -Wno-unused-parameter -Wvla
    BASE_CFLAGS = $(BASE_FLAGS) -std=c99 \
        -Wmissing-declarations -Wstrict-prototypes
    GCOV = llvm-cov
    GCOV_OPTS = -b -c
    GCOV_FILE_OPTS = \
        -gcno="`echo \"$1\" | sed -e 's|\.[^./]*$$|_cov.gcno|'`" \
        -gcda="`echo \"$1\" | sed -e 's|\.[^./]*$$|_cov.gcda|'`" \
        -o "`echo "$1" | sed -e 's|[/.]|-|g'`.out"
else ifeq ($(CC_TYPE),gcc)
    BASE_FLAGS = -O2 -pipe -g -I. \
        -Wall -Wextra $(call if-true,WARNINGS_AS_ERRORS,-Werror) \
        -Wcast-align -Winit-self -Wlogical-op -Wpointer-arith -Wshadow \
        -Wwrite-strings -Wundef -Wno-unused-parameter -Wvla
    BASE_CFLAGS = $(BASE_FLAGS) -std=c99 \
        -Wmissing-declarations -Wstrict-prototypes
    GCOV = gcov >/dev/null
    GCOV_OPTS = -b -c -l -p
    GCOV_FILE_OPTS = -o "`echo \"$1\" | sed -e 's|\.[^./]*$$|_cov.o|'`" '$1'
else ifeq ($(CC_TYPE),icc)
    BASE_FLAGS = -O2 -g -I. \
        $(call if-true,WARNINGS_AS_ERRORS,-Werror) \
        -Wpointer-arith -Wreturn-type -Wshadow -Wuninitialized \
        -Wunknown-pragmas -Wunused-function -Wunused-variable -Wwrite-strings
    BASE_CFLAGS = $(BASE_FLAGS) -std=c99 \
        -Wmissing-declarations -Wstrict-prototypes
else
    $(warning *** Warning: Unknown compiler type.)
    $(warning *** Make sure your CFLAGS are set correctly!)
endif


# Update build parameter defaults based on the compiler's target platform.

ifneq ($(filter clang gcc icc,$(CC_TYPE)),)
    TARGET := $(subst -, ,$(shell $(CC) -dumpmachine))
    ARCH := $(or $(firstword $(TARGET)),unknown)
    OSTYPE := $(or $(word 3,$(TARGET)),unknown)
else
    ARCH := unknown
    OSTYPE := unknown
endif

ifneq ($(filter darwin%,$(OSTYPE)),)
    SHARED_LIB_FULLNAME = $(subst .dylib,.$(VERSION).dylib,$(SHARED_LIB))
    SHARED_LIB_LINKNAME = $(subst .dylib,.$(VERSION_MAJOR).dylib,$(SHARED_LIB))
    SHARED_LIB_LDFLAGS = \
        -dynamiclib \
        -install_name '$(LIBDIR)/$(SHARED_LIB_FULLNAME)' \
        -compatibility_version $(firstword $(subst ., ,$(VERSION))) \
        -current_version $(VERSION) \
        -Wl,-single_module
else ifneq ($(filter mingw%,$(ARCH)),)
    SHARED_LIB_FULLNAME = $(SHARED_LIB)
    SHARED_LIB_LDFLAGS = \
        -shared \
        -Wl,--enable-auto-image-base \
        -Xlinker --out-implib -Xlinker '$@.a'
else
    SHARED_LIB_FULLNAME = $(SHARED_LIB).$(VERSION)
    SHARED_LIB_LINKNAME = $(SHARED_LIB).$(VERSION_MAJOR)
    SHARED_LIB_CFLAGS = -fPIC
    SHARED_LIB_LDFLAGS = -shared -Wl,-soname=$(SHARED_LIB_LINKNAME)
endif


# Final flag set.  Note that the user-specified $(CFLAGS) reference comes
# last so the user can override any of our default flags.

ALL_DEFS = $(strip \
    $(call define-if-true,ENABLE_ASSERT) \
    $(call define-if-true,ENABLE_OPERAND_SANITY_CHECKS) \
    -DVERSION=\"$(VERSION)\")

ALL_CFLAGS = $(BASE_CFLAGS) $(ALL_DEFS) $(CFLAGS)


# Libraries to use when linking test programs.

LIBS = -lm

###########################################################################
############################### Build rules ###############################
###########################################################################

#----------------------------- Entry points ------------------------------#

.PHONY: all all-shared all-static
.PHONY: install install-headers install-pc install-shared install-static install-tools
.PHONY: test clean spotless


all: $(call if-true,BUILD_SHARED,all-shared) \
     $(call if-true,BUILD_STATIC,all-static)

all-shared: $(SHARED_LIB)

all-static: $(STATIC_LIB)


install: $(call if-true,BUILD_SHARED,install-shared) \
         $(call if-true,BUILD_STATIC,install-static) \
         install-headers \
         $(call if-true,INSTALL_PKGCONFIG,install-pc)

install-headers:
	$(ECHO) 'Installing header files'
	$(Q)mkdir -p '$(DESTDIR)$(INCDIR)'
	$(Q)cp -pf include/binrec.h include/binrec++.h '$(DESTDIR)$(INCDIR)/'

install-pc:
	$(ECHO) 'Installing pkg-config control file'
	$(Q)mkdir -p '$(DESTDIR)$(LIBDIR)/pkgconfig'
	$(Q)sed \
	    -e 's|@PREFIX@|$(PREFIX)|g' \
	    -e 's|@INCDIR@|$(patsubst $(PREFIX)%,$${prefix}%,$(INCDIR))|g' \
	    -e 's|@LIBDIR@|$(patsubst $(PREFIX)%,$${prefix}%,$(LIBDIR))|g' \
	    -e 's|@VERSION@|$(VERSION)|g'\
	    <$(PACKAGE).pc.in >'$(DESTDIR)$(LIBDIR)/pkgconfig/$(PACKAGE).pc'

install-shared: all-shared
	$(ECHO) 'Installing shared library'
	$(Q)mkdir -p '$(DESTDIR)$(LIBDIR)'
	$(Q)cp -pf $(SHARED_LIB) '$(DESTDIR)$(LIBDIR)/$(SHARED_LIB_FULLNAME)'
	$(Q)$(if $(SHARED_LIB_LINKNAME),ln -sf '$(SHARED_LIB_FULLNAME)' '$(DESTDIR)$(LIBDIR)/$(SHARED_LIB_LINKNAME)')
	$(Q)$(if $(filter-out $(SHARED_LIB),$(SHARED_LIB_FULLNAME)),ln -sf '$(SHARED_LIB_FULLNAME)' '$(DESTDIR)$(LIBDIR)/$(SHARED_LIB)')

install-static: all-static
	$(ECHO) 'Installing static library'
	$(Q)mkdir -p '$(DESTDIR)$(LIBDIR)'
	$(Q)cp -pf $(STATIC_LIB) '$(DESTDIR)$(LIBDIR)/'


test: $(TEST_BINS)
	$(ECHO) 'Running tests'
	$(Q)ok=0 ng=0; \
	    for test in $^; do \
	        $(call if-true,V,echo "+ $${test}";) \
	        if "$${test}"; then \
	            ok=`expr $${ok} + 1`; \
	        else \
	            echo "FAIL: $${test}"; \
	            ng=`expr $${ng} + 1`; \
	        fi; \
	    done; \
	    if test $${ng} = 0; then \
	        echo 'All tests passed.'; \
	    else \
	        if test $${ok} = 1; then ok_s=''; else ok_s='s'; fi; \
	        if test $${ng} = 1; then ng_s=''; else ng_s='s'; fi; \
	        echo "$${ok} test$${ok_s} passed, $${ng} test$${ng_s} failed."; \
	        exit 1; \
	    fi

coverage: tests/coverage
	$(ECHO) 'Running tests'
	$(Q)find src -name \*.gcda -exec rm '{}' +
	$(Q)tests/coverage $(call if-true,V,-v)
	$(ECHO) 'Collecting coverage information'
	$(Q)rm -rf .covtmp
	$(Q)mkdir .covtmp
	$(Q)ln -s ../src .covtmp/
	$(Q)set -e; cd .covtmp && for f in $(LIBRARY_SOURCES); do \
	    $(call if-true,V,echo + $(GCOV) $(GCOV_OPTS) $(call GCOV_FILE_OPTS,$$f) $(GCOV_STDOUT);) \
	    $(GCOV) $(GCOV_OPTS) $(call GCOV_FILE_OPTS,$$f) $(GCOV_STDOUT); \
	done
	$(Q)rm -f .covtmp/src
	$(Q)cat .covtmp/* >coverage.txt
	$(Q)rm -rf .covtmp
	$(ECHO) 'Coverage results written to $(abspath coverage.txt)'


clean:
	$(ECHO) 'Removing object and dependency files'
	$(Q)find src tests \( -name '*.[do]' -o -name \*.d.tmp \) -exec rm '{}' +
	$(Q)rm -f tests/coverage-tests.h
	$(ECHO) 'Removing test executables'
	$(Q)rm -f $(TEST_BINS) tests/coverage
	$(ECHO) 'Removing coverage data files'
	$(Q)find src tests \( -name \*.gcda -o -name \*.gcno \) -exec rm '{}' +
	$(Q)rm -rf .covtmp

spotless: clean
	$(ECHO) 'Removing executable and library files'
	$(Q)rm -f $(SHARED_LIB) $(STATIC_LIB) $(BENCHMARK_BINS)
	$(ECHO) 'Removing coverage output'
	$(Q)rm -f coverage.txt

#-------------------------- Library build rules --------------------------#

$(SHARED_LIB): $(LIBRARY_OBJECTS:%.o=%_so.o)
	$(ECHO) 'Linking $@'
	$(Q)$(CC) $(SHARED_LIB_LDFLAGS) -o '$@' $^

$(STATIC_LIB): $(LIBRARY_OBJECTS)
	$(ECHO) 'Archiving $@'
	$(Q)$(AR) rcu '$@' $^
	$(Q)$(RANLIB) '$@'

#--------------------------- Test build rules ----------------------------#

TEST_UTILITY_OBJECTS = \
    tests/common.o \
    tests/execute.o \
    tests/log-capture.o \
    tests/mem-wrappers.o

$(TEST_BINS) : %: %.o $(TEST_UTILITY_OBJECTS) $(STATIC_LIB)
	$(ECHO) 'Linking $@'
	$(Q)$(CC) $(LDFLAGS) -o '$@' $^ $(LIBS)
	$(Q)$(call if-true,STRIP_TESTS,strip '$@')

tests/coverage: tests/coverage-main.o $(TEST_UTILITY_OBJECTS:%.o=%_cov.o) $(LIBRARY_OBJECTS:%.o=%_cov.o) $(TEST_SOURCES:%.c=%_cov.o)
	$(ECHO) 'Linking $@'
	$(Q)$(CC) $(ALL_CFLAGS) $(LDFLAGS) -o '$@' $^ $(LIBS) --coverage

tests/coverage-main.o: tests/coverage-tests.h

tests/coverage-tests.h: $(TEST_SOURCES)
	$(ECHO) 'Generating $@'
	$(Q)( \
	    for file in $(TEST_SOURCES:%.c=%); do \
	        file_mangled=_`echo "$${file}" | sed -e 's/[^A-Za-z0-9]/_/g'`; \
	        echo "TEST($${file_mangled})"; \
	    done \
	) >'$@'

#------------------------- Benchmark build rules -------------------------#

benchmarks/bench: $(BENCHMARK_OBJECTS) $(LIBRARY_OBJECTS)
	$(ECHO) 'Linking $@'
	$(Q)$(CC) $(ALL_CFLAGS) $(LDFLAGS) -o '$@' $^ $(LIBS)

benchmarks/dhrystone/%_noopt.o: benchmarks/dhrystone/%.c benchmarks/dhrystone/dhry.h
	$(ECHO) 'Compiling $< -> $@'
	$(Q)$(CC) -std=c89 -DBENCHMARK_ONLY -DDHRY_PREFIX=dhry_noopt_ \
	    -o '$@' -c '$<'

benchmarks/dhrystone/%_opt.o: benchmarks/dhrystone/%.c benchmarks/dhrystone/dhry.h
	$(ECHO) 'Compiling $< -> $@'
	$(Q)$(CC) -std=c89 -O2 -fno-inline \
	    -DBENCHMARK_ONLY -DDHRY_PREFIX=dhry_opt_ \
	    -o '$@' -c '$<'

#----------------------- Common compilation rules ------------------------#

%.o: %.c
	$(ECHO) 'Compiling $< -> $@'
	$(Q)$(CC) $(ALL_CFLAGS) -MMD -MF '$(@:%.o=%.d.tmp)' -o '$@' -c '$<'
	$(call filter-deps,$@,$(@:%.o=%.d))

# We generate separate dependency files for shared objects even though the
# contents are the same as for static objects to avoid parallel builds
# colliding when writing the dependencies.
%_so.o: BASE_CFLAGS += $(SHARED_LIB_CFLAGS)
%_so.o: %.c
	$(ECHO) 'Compiling $< -> $@'
	$(Q)$(CC) $(ALL_CFLAGS) -MMD -MF '$(@:%.o=%.d.tmp)' -o '$@' -c '$<'
	$(call filter-deps,$@,$(@:%.o=%.d))

src/%_cov.o: BASE_CFLAGS += -O0
src/%_cov.o: src/%.c
	$(ECHO) 'Compiling $< -> $@'
	$(Q)$(CC) -DCOVERAGE $(ALL_CFLAGS) --coverage -MMD -MF '$(@:%.o=%.d.tmp)' -o '$@' -c '$<'
	$(call filter-deps,$@,$(@:%.o=%.d))

tests/%_cov.o: tests/%.c
	$(ECHO) 'Compiling $< -> $@'
	$(Q)file_mangled=_`echo '$(<:%.c=%)' | sed -e 's/[^A-Za-z0-9]/_/g'`; \
	    $(CC) $(ALL_CFLAGS) "-Dmain=$${file_mangled}" \
	        $(if $(filter -Wmissing-declarations,$(ALL_CFLAGS)),-Wno-missing-declarations) \
	        -MMD -MF '$(@:%.o=%.d.tmp)' -o '$@' -c '$<'
	$(call filter-deps,$@,$(@:%.o=%.d))

#-------------------- Autogenerated dependency magic ---------------------#

define filter-deps
$(Q)test \! -f '$2.tmp' && rm -f '$2' || sed \
    -e ':1' \
    -e 's#\(\\ \|[^ /]\)\+/\.\./##' \
    -e 't1' \
    -e ':2' \
    -e 's#/\./#/#' \
    -e 't2' \
    -e 's#^\(\([^ 	]*[ 	]\)*\)$(subst .,\.,$(1:%$(OBJECT_EXT)=%.h))#\1#' \
    -e 's#$(subst .,\.,$1)#$1 $(1:%$(OBJECT_EXT)=%.h)#' \
    <'$2.tmp' >'$2~'
$(Q)rm -f '$2.tmp'
$(Q)mv '$2~' '$2'
endef

# Don't try to include dependency data if we're not actually building
# anything.  This is particularly important for "clean" and "spotless",
# since an out-of-date dependency file which referenced a nonexistent
# target (this can arise from switching versions in place using version
# control, for example) would otherwise block these targets from running
# and cleaning out that very dependency file!
ifneq ($(filter-out clean spotless,$(or $(MAKECMDGOALS),default)),)
include $(sort $(wildcard $(patsubst %.o,%.d,\
    $(LIBRARY_OBJECTS) \
    $(LIBRARY_OBJECTS:%.o=%_so.o) \
    $(LIBRARY_OBJECTS:%.o=%_cov.o) \
    $(TEST_OBJECTS) \
    $(TEST_OBJECTS:%.o=%_cov.o) \
    $(TEST_UTILITY_OBJECTS) \
    $(TEST_UTILITY_OBJECTS:%.o=%_cov.o) \
)))
endif

###########################################################################

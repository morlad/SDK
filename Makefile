# Makefile to generate a statically linked modio shared library


# OS DETECTION
# ------------
ifdef ComSpec
	os = windows
else
	uname_s = $(shell uname -s)
	ifeq ($(uname_s),Darwin)
		os = osx
	endif
	ifeq ($(uname_s),Linux)
		os = linux
	endif
	ifeq ($(uname_s),FreeBSD)
		os = freebsd
	endif
endif


# CONFIG
# ------
OUTPUT_DIR := build

# macos only:
MIN_MACOS_VERSION = 10.13

# windows only:
# path to the LLVM installation
# ! do NOT add trailing \ 
# ! do NOT quote it
LLVM_PATH = C:\Program Files\LLVM

OPT = -O2
OPT += -fstrict-aliasing
CURL_VERSION = tags/curl-7_65_0 
JSON_VERSION = tags/v3.6.1

USE_SANITIZER = 0
Q = @

ifeq ($(os),osx)
LIBRARY_NAME = libmodio.dylib
endif
ifeq ($(os),windows)
LIBRARY_NAME = modio.dll
endif
ifeq ($(os),linux)
LIBRARY_NAME = libmodio.so
endif

ifneq ($(os),linux)
WARNINGS += -Weverything
WARNINGS += -Werror-shadow
# curl:
NOWARNINGS += -Wno-reserved-id-macro
NOWARNINGS += -Wno-documentation-unknown-command
NOWARNINGS += -Wno-nonportable-system-include-path
# everything:
NOWARNINGS += -Wno-zero-as-null-pointer-constant
# temporary:
NOWARNINGS += -Wno-exit-time-destructors
NOWARNINGS += -Wno-global-constructors
# json, macos:
NOWARNINGS += -Wno-switch-enum
NOWARNINGS += -Wno-covered-switch-default
NOWARNINGS += -Wno-weak-vtables
NOWARNINGS += -Wno-padded
else
WARNINGS += -Wall
endif


# SOURCE FILES
# ------------
deps_src += \
	miniz/miniz.c

deps_src += \
	dirent/dirent.cpp

deps_src += \
	minizip/crypt.cpp \
	minizip/ioapi.cpp \
	minizip/minizip.cpp \
	minizip/unzip.cpp \
	minizip/zip.cpp

deps_src += \
	curl/lib/asyn-thread.c \
	curl/lib/base64.c \
	curl/lib/conncache.c \
	curl/lib/connect.c \
	curl/lib/content_encoding.c \
	curl/lib/cookie.c \
	curl/lib/curl_addrinfo.c \
	curl/lib/curl_ctype.c \
	curl/lib/curl_des.c \
	curl/lib/curl_endian.c \
	curl/lib/curl_get_line.c \
	curl/lib/curl_gethostname.c \
	curl/lib/curl_memrchr.c \
	curl/lib/curl_ntlm_core.c \
	curl/lib/curl_threads.c \
	curl/lib/doh.c \
	curl/lib/dotdot.c \
	curl/lib/easy.c \
	curl/lib/escape.c \
	curl/lib/formdata.c \
	curl/lib/getenv.c \
	curl/lib/getinfo.c \
	curl/lib/hash.c \
	curl/lib/hmac.c \
	curl/lib/hostasyn.c \
	curl/lib/hostip.c \
	curl/lib/hostip4.c \
	curl/lib/hostip6.c \
	curl/lib/http.c \
	curl/lib/http_chunks.c \
	curl/lib/http_digest.c \
	curl/lib/http_ntlm.c \
	curl/lib/http_proxy.c \
	curl/lib/if2ip.c \
	curl/lib/llist.c \
	curl/lib/md5.c \
	curl/lib/mime.c \
	curl/lib/mprintf.c \
	curl/lib/multi.c \
	curl/lib/netrc.c \
	curl/lib/nonblock.c \
	curl/lib/parsedate.c \
	curl/lib/progress.c \
	curl/lib/rand.c \
	curl/lib/select.c \
	curl/lib/sendf.c \
	curl/lib/setopt.c \
	curl/lib/sha256.c \
	curl/lib/share.c \
	curl/lib/slist.c \
	curl/lib/socks.c \
	curl/lib/speedcheck.c \
	curl/lib/splay.c \
	curl/lib/strcase.c \
	curl/lib/strdup.c \
	curl/lib/strerror.c \
	curl/lib/strtoofft.c \
	curl/lib/timeval.c \
	curl/lib/transfer.c \
	curl/lib/url.c \
	curl/lib/urlapi.c \
	curl/lib/vauth/digest.c \
	curl/lib/vauth/ntlm.c \
	curl/lib/vauth/vauth.c \
	curl/lib/version.c \
	curl/lib/vtls/vtls.c \
	curl/lib/warnless.c \

ifeq ($(os),osx)
deps_src += \
	curl/lib/vtls/sectransp.c \

endif
	

ifeq ($(os),windows)
deps_src += \
	curl/lib/curl_multibyte.c \
	curl/lib/curl_sspi.c \
	curl/lib/hostcheck.c \
	curl/lib/http_negotiate.c \
	curl/lib/inet_ntop.c \
	curl/lib/inet_pton.c \
	curl/lib/socks_sspi.c \
	curl/lib/strtok.c \
	curl/lib/system_win32.c \
	curl/lib/vauth/digest_sspi.c \
	curl/lib/vauth/ntlm_sspi.c \
	curl/lib/vauth/spnego_sspi.c \
	curl/lib/vtls/schannel.c \
	curl/lib/vtls/schannel_verify.c \
	curl/lib/x509asn1.c \

endif

# add modio source files via wildcards
modio_src += $(wildcard src/*.cpp)

modio_src += $(wildcard src/c/creators/*.cpp)
modio_src += $(wildcard src/c/methods/*.cpp)
modio_src += $(wildcard src/c/methods/callbacks/*.cpp)
modio_src += $(wildcard src/c/schemas/*.cpp)

modio_src += $(wildcard src/c++/creators/*.cpp)
modio_src += $(wildcard src/c++/methods/*.cpp)
modio_src += $(wildcard src/c++/methods/callbacks/*.cpp)
modio_src += $(wildcard src/c++/schemas/*.cpp)

modio_src += $(wildcard src/wrappers/*.cpp)

$(OUTPUT_DIR)/amalgated.cpp: Makefile $(modio_src)
ifdef Q
	@echo Amalgating source files $@
endif
	$(Q)echo // Amalgated file > $@
ifeq ($(os),windows)
	$(Q)type src\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\c\creators\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\c\methods\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\c\methods\callbacks\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\c\schemas\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\c++\creators\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\c++\methods\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\c++\methods\callbacks\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\c++\schemas\*.cpp >> $@ 2> $(DEVNULL)
	$(Q)type src\wrappers\*.cpp >> $@ 2> $(DEVNULL)
else
	$(Q)cat src/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/c/creators/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/c/methods/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/c/methods/callbacks/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/c/schemas/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/c++/creators/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/c++/methods/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/c++/methods/callbacks/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/c++/schemas/*.cpp >> $@ 2> $(DEVNULL)
	$(Q)cat src/wrappers/*.cpp >> $@ 2> $(DEVNULL)
endif


# OBJECT FILES
# ------------
deps_o += $(subst .c,.o,$(addprefix $(OUTPUT_DIR)/dependencies/,$(filter %.c,$(deps_src))))
deps_o += $(subst .cpp,.o,$(addprefix $(OUTPUT_DIR)/dependencies/,$(filter %.cpp,$(deps_src))))
modio_o += $(OUTPUT_DIR)/amalgated.o


# LINKER OPTIONS
# --------------
TARGET_ARCH = -m64 -g -march=core2

ifeq ($(os),osx)
TARGET_ARCH += -arch x86_64 -mmacosx-version-min=$(MIN_MACOS_VERSION) -stdlib=libc++
LDLIBS += -lc++ -framework Security
ifeq ($(USE_SANITIZER),1)
TARGET_ARCH += -fsanitize=address
TARGET_ARCH += -fsanitize=undefined
endif
endif

ifeq ($(os),windows)
TARGET_ARCH += -gcodeview
LDFLAGS += /NOLOGO /MACHINE:X64 /NODEFAULTLIB /INCREMENTAL:NO
LDFLAGS += /SUBSYSTEM:WINDOWS /DEBUG
LDLIBS += libucrt.lib
LDLIBS += libvcruntime.lib
LDLIBS += libcmt.lib
LDLIBS += libcpmt.lib
LDLIBS += ws2_32.lib
LDLIBS += kernel32.lib
LDLIBS += advapi32.lib
LDLIBS += crypt32.lib
endif

ifeq ($(os),linux)
LDLIBS += -lstdc++ -lpthread
endif

# COMPILER OPTIONS
# ----------------
CFLAGS += -std=c11
CFLAGS += $(WARNINGS) $(NOWARNINGS) $(OPT)
CXXFLAGS += -std=c++17
CXXFLAGS += $(WARNINGS) $(NOWARNINGS) $(OPT)
# implies clang
ifneq ($(os),linux)
CXXFLAGS += -Weffc++ -Wno-c++98-compat -Wno-c++98-compat-pedantic
endif

ifeq ($(os),windows)
CPPFLAGS += -DUNICODE -D_UNICODE
CPPFLAGS += -DNDEBUG
CPPFLAGS += -D_CRT_SECURE_NO_WARNINGS
CPPFLAGS += -DCURL_STATICLIB
CPPFLAGS += -DHAS_STDINT_H
endif

ifeq ($(os),linux)
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -fPIC
CXXFLAGS += -fPIC
endif

# TARGETS
# -------
all: library

library: $(OUTPUT_DIR)/$(LIBRARY_NAME)

clean-lib:
	$(Q)$(RM) $(OUTPUT_DIR)/$(LIBRARY_NAME)

dependencies: $(deps_o)
modio: $(modio_o)

$(OUTPUT_DIR)/$(LIBRARY_NAME): $(deps_o) $(modio_o)
ifdef Q
	@echo Linking $@
endif
	-$(Q)$(ensure_dir)

ifeq ($(os),osx)
	$(Q)$(CC) -dynamiclib $(TARGET_ARCH) $(LDFLAGS) $^ $(LDLIBS) $(OUTPUT_OPTION) -Wl,-install_name,@loader_path/$(notdir $@)
endif

# linker command line exceeds what windows can handle, so split object
# files and load them via @ into the linker
# thank you windows...
ifeq ($(os),windows)
	$(Q)del $(OUTPUT_DIR)\linker-input-files-1.txt
	$(Q)echo $(deps_o) >> $(OUTPUT_DIR)\linker-input-files-1.txt
	$(Q)del $(OUTPUT_DIR)\linker-input-files-2.txt
	$(Q)echo $(modio_o) >> $(OUTPUT_DIR)\linker-input-files-2.txt
	$(Q)$(LINKER) /DLL $(LDFLAGS) $(LDLIBS) /OUT:$(subst /,\,$@) @$(OUTPUT_DIR)\linker-input-files-1.txt @$(OUTPUT_DIR)\linker-input-files-2.txt
endif

ifeq ($(os),linux)
	$(Q)$(CC) -shared $(TARGET_ARCH) $(LDFLAGS) $^ $(LDLIBS) $(OUTPUT_OPTION)
	$(Q)strip --strip-debug $@
endif


fetch-curl:
	-$(Q)git clone https://github.com/curl/curl.git dependencies/curl
	$(Q)git -C dependencies/curl fetch --all --tags
	$(Q)git -C dependencies/curl checkout $(CURL_VERSION)

fetch-json:
	-$(Q)git clone https://github.com/nlohmann/json.git dependencies/json
	$(Q)git -C dependencies/json fetch --all --tags
	$(Q)git -C dependencies/json checkout $(JSON_VERSION)

fetch-all: fetch-curl fetch-json

# SPECIAL FILE HANDLING
# ---------------------
$(OUTPUT_DIR)/src/%.o: CPPFLAGS += -Iinclude -Idependencies/curl/include -Idependencies/miniz -Idependencies/json/single_include -Idependencies

$(OUTPUT_DIR)/amalgated.o: CPPFLAGS += -Iinclude -Idependencies/curl/include -Idependencies/miniz -Idependencies/json/single_include -Idependencies


# clang implied
ifneq ($(os),linux)
$(OUTPUT_DIR)/src/wrappers/Curl%.o: NOWARNINGS += -Wno-disabled-macro-expansion
$(OUTPUT_DIR)/dependencies/%.o: NOWARNINGS += -Wno-everything
$(OUTPUT_DIR)/amalgated.o: NOWARNINGS += -Wno-old-style-cast -Wno-disabled-macro-expansion -Wno-unused-parameter
endif

$(OUTPUT_DIR)/dependencies/minizip/%.o: CPPFLAGS += -Idependencies -Idependencies/miniz

$(OUTPUT_DIR)/dependencies/curl/lib/%.o: CPPFLAGS += -Idependencies/curl/include -Idependencies/curl/lib -Idependencies/miniz
$(OUTPUT_DIR)/dependencies/curl/lib/%.o: CPPFLAGS += -DBUILDING_LIBCURL -DCURL_NO_OLDIES -DHTTP_ONLY

ifeq ($(os),osx)
$(OUTPUT_DIR)/dependencies/curl/lib/%.o: CPPFLAGS += -DHAVE_CONFIG_H -Iinclude/dependencies/curl/macos
# use Security-framework as TLS-backend
$(OUTPUT_DIR)/dependencies/curl/lib/%.o: CPPFLAGS += -DUSE_SECTRANSP
endif

ifeq ($(os),linux)
$(OUTPUT_DIR)/dependencies/curl/lib/%.o: CPPFLAGS += -DHAVE_CONFIG_H -Iinclude/dependencies/curl/linux
endif

ifeq ($(os),windows)
$(OUTPUT_DIR)/src/%.o: CPPFLAGS += -DMODIO_DYNAMICLIB
$(OUTPUT_DIR)/amalgated.o: CPPFLAGS += -DMODIO_DYNAMICLIB
$(OUTPUT_DIR)/dependencies/curl/lib/%.o: CPPFLAGS += -Daccess=_access -Dread=_read -Dwrite=_write
# use Schannel-framework as TLS-backend
$(OUTPUT_DIR)/dependencies/curl/lib/%.o: CPPFLAGS += -DUSE_SCHANNEL -DUSE_WINDOWS_SSPI
endif


# MACROS/HELPERS
# --------------
ifeq ($(os),osx)
	DEVNULL = /dev/null
	ensure_dir=mkdir -p $(@D) 2> $(DEVNULL) || exit 0
	RM = rm
endif
ifeq ($(os),windows)
	DEVNULL = NUL
	CC = "$(LLVM_PATH)\bin\clang.exe"
	CXX = "$(LLVM_PATH)\bin\clang++.exe"
	LINKER = "$(LLVM_PATH)\bin\lld-link.exe"
	ensure_dir=mkdir $(subst /,\,$(@D)) 2> $(DEVNULL) || exit 0
	RM = del
endif
ifeq ($(os),linux)
	DEVNULL = /dev/null
	ensure_dir=mkdir -p $(@D) 2> $(DEVNULL) || exit 0
	RM = rm
endif


# PATTERNS
# --------
$(OUTPUT_DIR)/%.o: %.c
ifdef Q
	@echo CC $<
endif
	-$(Q)$(ensure_dir)
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(TARGET_ARCH) -c $(OUTPUT_OPTION) $<

$(OUTPUT_DIR)/%.o: %.cpp
ifdef Q
	@echo CXX $<
endif
	-$(Q)$(ensure_dir)
	$(Q)$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TARGET_ARCH) -c $(OUTPUT_OPTION) $<


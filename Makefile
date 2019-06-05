# Makefile to generate a statically linked modio shared library


# OS DETECTION (don't touch)
# --------------------------
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
USE_SANITIZER = 0
MIN_MACOS_VERSION = 10.13
Q = @
OPT = -O2
OPT += -fstrict-aliasing
CURL_VERSION = tags/curl-7_65_0 
ifeq ($(os),osx)
LIBRARY_NAME = libmodio.dylib
endif
ifeq ($(os),windows)
LIBRARY_NAME = modio.dll
endif
ifeq ($(os),linux)
LIBRARY_NAME = libmodio.so
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
	curl/lib/curl_get_line.c \
	curl/lib/curl_gethostname.c \
	curl/lib/curl_endian.c \
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
	curl/lib/hostasyn.c \
	curl/lib/hostip.c \
	curl/lib/hostip4.c \
	curl/lib/hostip6.c \
	curl/lib/hmac.c \
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
	curl/lib/vauth/vauth.c \
	curl/lib/vauth/ntlm.c \
	curl/lib/vtls/vtls.c \
	curl/lib/vtls/sectransp.c \
	curl/lib/version.c \
	curl/lib/warnless.c \


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


# OBJECT FILES (don't touch)
# --------------------------
deps_o += $(subst .c,.o,$(addprefix $(OUTPUT_DIR)/src/dependencies/,$(filter %.c,$(deps_src))))
deps_o += $(subst .cpp,.o,$(addprefix $(OUTPUT_DIR)/src/dependencies/,$(filter %.cpp,$(deps_src))))
modio_o += $(subst .c,.o,$(addprefix $(OUTPUT_DIR)/,$(filter %.c,$(modio_src))))
modio_o += $(subst .cpp,.o,$(addprefix $(OUTPUT_DIR)/,$(filter %.cpp,$(modio_src))))


# MANDATORY COMPILE/LINK OPTIONS (don't touch)
# --------------------------------------------
TARGET_ARCH = -m64 -g -march=core2

ifeq ($(os),osx)
TARGET_ARCH += -arch x86_64 -mmacosx-version-min=$(MIN_MACOS_VERSION) -stdlib=libc++
LDLIBS += -lc++ -framework Security
ifeq ($(USE_SANITIZER),1)
TARGET_ARCH += -fsanitize=address
TARGET_ARCH += -fsanitize=undefined
endif
endif

CFLAGS += -std=c11
CFLAGS += $(WARNINGS) $(NOWARNINGS) $(OPT)
CXXFLAGS += -std=c++11 -fno-rtti
CXXFLAGS += $(WARNINGS) -Weffc++ $(NOWARNINGS) -Wno-c++98-compat -Wno-c++98-compat-pedantic $(OPT)


# TARGETS
# -------
all: library

library: $(OUTPUT_DIR)/$(LIBRARY_NAME)

clean-lib:
	$(Q)$(RM) $(OUTPUT_DIR)/$(LIBRARY_NAME)

$(OUTPUT_DIR)/$(LIBRARY_NAME): $(deps_o) $(modio_o)
ifdef Q
	@echo Linking $@
endif
	$(Q)$(ensure_dir)
ifeq ($(os),osx)
	$(Q)$(CC) -dynamiclib $(TARGET_ARCH) $(LDFLAGS) $^ $(LDLIBS) $(OUTPUT_OPTION) -Wl,-install_name,@loader_path/$(notdir $@)
endif
ifeq ($(os),windows)
endif
ifeq ($(os),linux)
endif

fetch-curl:
	-$(Q)git clone https://github.com/curl/curl.git src/dependencies/curl
	$(Q)git -C src/dependencies/curl fetch --all --tags
	$(Q)git -C src/dependencies/curl checkout $(CURL_VERSION)


# SPECIAL FILE HANDLING
# ---------------------
$(OUTPUT_DIR)/src/%.o: CPPFLAGS += -Iinclude -Isrc/dependencies/curl/include

$(OUTPUT_DIR)/src/dependencies/miniz/miniz.o: CPPFLAGS += -Iinclude/dependencies/miniz

$(OUTPUT_DIR)/src/dependencies/minizip/%.o: CPPFLAGS += -Iinclude/dependencies -Iinclude/dependencies/miniz

$(OUTPUT_DIR)/src/dependencies/curl/lib/%.o: CPPFLAGS += -Isrc/dependencies/curl/include -Isrc/dependencies/curl/lib -Iinclude/dependencies/curl/macos -Iinclude/dependencies/miniz
$(OUTPUT_DIR)/src/dependencies/curl/lib/%.o: CPPFLAGS += -DHAVE_CONFIG_H -DCURL_STATICLIB -DBUILDING_LIBCURL -DCURL_NO_OLDIES -DHTTP_ONLY

ifeq ($(os),osx)
# use Security-framework as TLS-backend
$(OUTPUT_DIR)/src/dependencies/curl/lib/%.o: CPPFLAGS += -DUSE_SECTRANSP
endif


# MACROS/HELPERS
# --------------
ensure_dir=mkdir -p $(@D)
ifeq ($(os),osx)
	RM = rm
endif
ifeq ($(os),windows)
	RM = del
endif
ifeq ($(os),linux)
	RM = rm
endif


# PATTERNS
# --------
$(OUTPUT_DIR)/%.o: %.c
ifdef Q
	@echo CC $<
endif
	$(Q)$(ensure_dir)
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(TARGET_ARCH) -c $(OUTPUT_OPTION) $<

$(OUTPUT_DIR)/%.o: %.cpp
ifdef Q
	@echo CXX $<
endif
	$(Q)$(ensure_dir)
	$(Q)$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TARGET_ARCH) -c $(OUTPUT_OPTION) $<


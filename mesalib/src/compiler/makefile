include Makefile.sources

top_srcdir=../..
srcdir=.
builddir=.

INCLUDES += \
	$(top_srcdir)/include \
	$(top_srcdir)/src \
	$(top_srcdir)/src/mapi \
	$(top_srcdir)/src/mesa/ \
	$(top_builddir)/src/compiler/glsl\
	$(top_srcdir)/src/compiler/glsl\
	$(top_srcdir)/src/compiler/glsl/glcpp\
	$(top_builddir)/src/compiler/nir \
	$(top_srcdir)/src/compiler/nir \
	$(top_srcdir)/src/compiler/spirv \
	$(top_srcdir)/src/gallium/include \
	$(top_srcdir)/src/gallium/auxiliary \
	$(top_srcdir)/src/gtest/include \

libcompiler_la_SOURCES = $(LIBCOMPILER_FILES)

glsl_libglcpp_la_SOURCES =				\
	glsl/glcpp/glcpp-lex.c				\
	glsl/glcpp/glcpp-parse.c			\
	glsl/glcpp/glcpp-parse.h			\
	$(LIBGLCPP_FILES)

glsl_libglsl_la_SOURCES =				\
	$(LIBGLSL_GENERATED_FILES)			\
	$(LIBGLSL_FILES)				\
	$(LIBGLSL_SHADER_CACHE_FILES)

INCLUDES += \
	$(top_builddir)/src/compiler/nir		\
	$(top_srcdir)/src/compiler/nir		\
	$(top_srcdir)/src/compiler/spirv

DEFINES = INSERVER _USE_MATH_DEFINES __STDC_CONSTANT_MACROS __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS XML_STATIC

LIBRARY = libcompiler

CSRCS := $(notdir $(subst /,$/,$(libcompiler_la_SOURCES)))
CSRCS += $(notdir $(subst /,$/,$(glsl_libglsl_la_SOURCES)))
CSRCS += $(notdir $(subst /,$/,$(glsl_libglcpp_la_SOURCES)))
CSRCS := $(CSRCS:%.h=)

vpath %.c glsl:glsl/glcpp
vpath %.cpp glsl

load_makefile -f makefile.srcs NORELDBG=1

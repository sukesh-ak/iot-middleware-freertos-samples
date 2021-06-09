#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_ADD_INCLUDEDIRS += . include esp32_tls_transport
COMPONENT_SRCDIRS := . esp32_tls_transport
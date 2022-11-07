# Additional FLAGS and KWARGS for custom commands
additional_commands = {
  "dabc_link_library": {
    "pargs": 1,
    "flags": ["NOWARN", "DABC_INSTALL"],
    "kwargs": {
      "SOURCES": '*',
      "HEADERS": '*',
      "PUBLIC_HEADERS": '*',
      "INCLUDE_SUBDIR": '*',
      "LIBRARIES": '*',
      "DEFINITIONS": '*',
      "DEPENDENCIES": '*',
      "INCLUDES": '*',
    }
  },
  "dabc_executable": {
    "pargs": 1,
    "flags": ["CHECKSTD"],
    "kwargs": {
      "SOURCES": '*',
      "LIBRARIES": '*',
      "INCLUDES": '*',
    }
  },
  "root_generate_dictionary": {
    "pargs": 1,
    "flags": ["NOINSTALL"],
    "kwargs": {
      "MODULE": '*',
      "LINKDEF": '*',
    }
  }
}

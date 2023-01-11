# Additional FLAGS and KWARGS for custom commands
additional_commands = {
  "dabc_link_library": {
    "pargs": 1,
    "flags": ["NOWARN", "DABC_INSTALL"],
    "kwargs": {
      "SOURCES": '*',
      "HEADERS": '*',
      "PUBLIC_HEADERS": '*',
      "INCDIR": '*',
      "LIBRARIES": '*',
      "DEFINITIONS": '*',
      "DEPENDENCIES": '*',
      "INCLUDES": '*',
    }
  },
  "dabc_install_plugin_data": {
    "pargs": 1,
    "flags": [],
    "kwargs": {
      "FILES": '*',
      "DIRECTORIES": '*',
      "DESTINATION": '*',
      "LEGACY_MODE": '*',
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

# list of DABC plugins

if(mbs)
  add_subdirectory(mbs)
endif()

if(hadaq)
  add_subdirectory(hadaq)
endif()

if(dogma)
  add_subdirectory(dogma)
endif()

if(verbs)
  add_subdirectory(verbs)
endif()

if(http)
  add_subdirectory(http)
endif()

if(root)
  add_subdirectory(root)
  if(mbsroot)
    add_subdirectory(mbs-root)
  endif()
endif()

if(stream)
  add_subdirectory(stream)
endif()


if(elder)
  add_subdirectory(elder)
endif()

if(aqua)
  add_subdirectory(aqua)
endif()

if(bnet)
  add_subdirectory(bnet)
endif()

if(dim)
  add_subdirectory(dim)
endif()

if(ezca)
  add_subdirectory(ezca)
endif()

if(fesa)
  add_subdirectory(fesa)
endif()

if(gosip)
  add_subdirectory(gosip)
endif()

if(dofi)
  add_subdirectory(dofi)
endif()

if(ltsm)
  add_subdirectory(ltsm)
endif()

if(rfio)
  add_subdirectory(rfio)
endif()

if(saft)
  add_subdirectory(saft)
endif()

if(olmd)
  add_subdirectory(olmd)
endif()

add_subdirectory(user)

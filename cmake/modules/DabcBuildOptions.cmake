
set(dabc_build_options)

#---------------------------------------------------------------------------------------------------
#---DABC_BUILD_OPTION( name defvalue [description] )
#---------------------------------------------------------------------------------------------------
function(DABC_BUILD_OPTION opt defvalue)
  if(ARGN)
    set(description ${ARGN})
  else()
    set(description " ")
  endif()
  set(${opt}_defvalue    ${defvalue} PARENT_SCOPE)
  set(${opt}_description ${description} PARENT_SCOPE)
  set(dabc_build_options  ${dabc_build_options} ${opt} PARENT_SCOPE)
endfunction()

#---------------------------------------------------------------------------------------------------
#---DABC_APPLY_OPTIONS()
#---------------------------------------------------------------------------------------------------
function(DABC_APPLY_OPTIONS)
  foreach(opt ${dabc_build_options})
    option(${opt} "${${opt}_description}" ${${opt}_defvalue})
  endforeach()
endfunction()

#---------------------------------------------------------------------------------------------------
#---DABC_GET_OPTIONS(result ENABLED)
#---------------------------------------------------------------------------------------------------
function(DABC_GET_OPTIONS result)
  cmake_parse_arguments(ARG "ENABLED" "" "" ${ARGN})
  set(enabled)
  foreach(opt ${dabc_build_options})
    if(ARG_ENABLED)
      if(${opt})
        set(enabled "${enabled} ${opt}")
      endif()
    else()
      set(enabled "${enabled} ${opt}")
    endif()
  endforeach()
  set(${result} "${enabled}" PARENT_SCOPE)
endfunction()

#---------------------------------------------------------------------------------------------------
#---DABC_SHOW_ENABLED_OPTIONS()
#---------------------------------------------------------------------------------------------------
function(DABC_SHOW_ENABLED_OPTIONS)
  set(enabled_opts)
  dabc_get_options(enabled_opts ENABLED)
  foreach(opt ${enabled_opts})
    message(STATUS "Enabled support for: ${opt}")
  endforeach()
endfunction()

#--------------------------------------------------------------------------------------------------
#---Full list of options with their descriptions and default values
#   The default value can be changed as many times as we wish before calling DABC_APPLY_OPTIONS()
#--------------------------------------------------------------------------------------------------

dabc_build_option(aqua ON "Enable AQUA plugin")
dabc_build_option(bnet ON "Enable BNET plugin")
dabc_build_option(dim OFF "Enable DIM plugin")
dabc_build_option(ezca OFF "Enable EZCA (EPICS) plugin")
dabc_build_option(fesa ON "Enable FESA plugin")
dabc_build_option(gosip OFF "Enable GOSIP plugin")
dabc_build_option(hadaq ON "Enable HADAQ plugin")
dabc_build_option(http ON "Enable HTTP plugin")
dabc_build_option(ltsm OFF "Enable LTSM plugin")
dabc_build_option(mbs ON "Enable MBS plugin")
dabc_build_option(rfio ON "Enable RFIO plugin")
dabc_build_option(saft OFF "Enable SAFT plugin")
dabc_build_option(stream ON "Enable Stream plugin")
dabc_build_option(user ON "Enable USER plugin")
dabc_build_option(verbs ON "Enable VERBS plugin")
dabc_build_option(root ON "Enable ROOT plugin")
dabc_build_option(mbsroot ON "Enable MBS-ROOT plugin")

#--- The 'all' option switches ON major options---------------------------------------------------
if(all)
  set(aqua_defvalue ON)
  set(bnet_defvalue ON)
  set(dim_defvalue ON)
  set(ezca_defvalue ON)
  set(fesa_defvalue ON)
  set(gosip_defvalue ON)
  set(hadaq_defvalue ON)
  set(http_defvalue ON)
  set(ltsm_defvalue ON)
  set(mbs_defvalue ON)
  set(rfio_defvalue ON)
  set(saft_defvalue ON)
  set(stream_defvalue ON)
  set(user_defvalue ON)
  set(verbs_defvalue ON)
  set(root_defvalue ON)
  set(mbsroot_defvalue ON)
endif()

#--- The 'all' option switches ON major options---------------------------------------------------
if(minimal)
  set(aqua_defvalue OFF)
  set(bnet_defvalue OFF)
  set(dim_defvalue OFF)
  set(ezca_defvalue OFF)
  set(fesa_defvalue OFF)
  set(gosip_defvalue OFF)
  set(hadaq_defvalue ON)
  set(http_defvalue ON)
  set(ltsm_defvalue OFF)
  set(mbs_defvalue ON)
  set(rfio_defvalue ON)
  set(saft_defvalue OFF)
  set(stream_defvalue OFF)
  set(user_defvalue OFF)
  set(verbs_defvalue OFF)
  set(root_defvalue OFF)
  set(mbsroot_defvalue OFF)
endif()

#---Define at moment the options with the selected default values-----------------------------
dabc_apply_options()

#---RPATH options-------------------------------------------------------------------------------
#  When building, don't use the install RPATH already (but later on when installing)
list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
set(CMAKE_SKIP_BUILD_RPATH FALSE)         # don't skip the full RPATH for the build tree
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) # use always the build RPATH for the build tree
set(CMAKE_MACOSX_RPATH TRUE)              # use RPATH for MacOSX
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) # point to directories outside the build tree to the install RPATH

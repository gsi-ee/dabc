# Find SPI includes required for DOFI

set(DOFI_FOUND 0)

find_path(DOFI_INCLUDE_DIR spi/spidev.h /usr/include/linux)

    if (DOFI_INCLUDE_DIR)
      set(DOFI_FOUND 1)
      message(STATUS "Found SPI includes in ${DOFI_INCLUDE_DIR} ")
   	endif()

mark_as_advanced(DOFI_FOUND DOFI_INCLUDE_DIR)


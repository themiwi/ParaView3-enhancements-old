#
## Copyright 2003 Sandia Coporation
## Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
## license for use of this work by or on behalf of the U.S. Government.
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that this Notice and any statement
## of authorship are reproduced on all copies.
#
# $Id: FindIceT.cmake,v 1.1 2003-06-19 18:08:12 kmorel Exp $
#
# Find an ICE-T installation or build tree.
#
# The following variables are set if ICE-T is found.  If ICE-T is not found,
# ICET_FOUND is set to false.
#
# ICET_FOUND		- Set to true when ICE-T is found.
# ICET_USE_FILE		- CMake source file to setup a project to use ICE-T
# ICET_MAJOR_VERSION	- The ICE-T major version number.
# ICET_MINOR_VERSION	- The ICE-T minor version number.
# ICET_PATCH_VERSION	- The ICE-T patch version number.
# ICET_INCLUDE_DIRS	- Include directories for ICE-T headers.
# ICET_LIBRARY_DIRS	- Link directories for ICE-T libraries.
#
# The following cache entries must be set by the user to locate ICE-T:
#
# ICET_DIR		- The directory containing ICETConfig.cmake.
#

SET(ICET_DIR_DESCRIPTION "directory containing ICETConfig.cmake.  This is either the root of the build tree, or PREFIX/lib for an installation.")
SET(ICET_DIR_MESSAGE "ICE-T not found.  Set ICET_DIR to the ${ICET_DIR_DESCRIPTION}")

IF (NOT ICET_DIR)
  # Get the system search path as a list.
  IF(UNIX)
    STRING(REGEX MATCHALL "[^:]+" ICET_DIR_SEARCH1 "$ENV{PATH}")
  ELSE(UNIX)
    STRING(REGEX REPLACE "\\\\" "/" ICET_DIR_SEARCH1 "$ENV{PATH}")
  ENDIF(UNIX)
  STRING(REGEX REPLACE "/;" ";" ICET_DIR_SEARCH2 "${ICET_DIR_SEARCH1}")

  # Construct a set of paths relative to the system search path.
  SET(ICET_DIR_SEARCH "")
  FOREACH(dir ${ICET_DIR_SEARCH2})
    SET(ICET_DIR_SEARCH ${ICET_DIR_SEARCH} "${dir}/../lib")
  ENDFOREACH(dir)

  FIND_PATH(ICET_DIR ICETConfig.cmake
	# Look in places relative to the system executable search path.
	${ICET_DIR_SEARCH}
	# Look in standard UNIX install locations.
	/usr/local/lib
	/usr/lib
	# Look in standard Win32 install locations.
	"C:/Program Files/ICE-T/lib"
	# Give documentation to user in case we can't find it.
	DOC "The ${ICET_DIR_DESCRIPTION}")
ENDIF (NOT ICET_DIR)

# If ICE-T was found, load the configuration file to get the rest of the
# settings.
IF(ICET_DIR)
  # Make sure the ICETConfig.cmake file exists in the directory provided.
  IF(EXISTS ${ICET_DIR}/ICETConfig.cmake)

    # We found ICE-T.  Load the settings.
    SET(ICET_FOUND 1)
    INCLUDE(${ICET_DIR}/ICETConfig.cmake)

  ELSE(EXISTS ${ICET_DIR}/ICETConfig.cmake)
    # We did not find ICE-T.
    SET(ICET_FOUND 0)
  ENDIF(EXISTS ${ICET_DIR}/ICETConfig.cmake)
ELSE(ICET_DIR)
  # We did not find ICE-T.
  SET(ICET_FOUND 0)
ENDIF(ICET_DIR)

#-----------------------------------------------------------------------------
IF(NOT ICET_FOUND)
  # ICE-T not found, explain to the user how to specify its location.
  IF(NOT ICET_FIND_QUIETLY)
    MESSAGE(${ICET_DIR_MESSAGE})
  ENDIF(NOT ICET_FIND_QUIETLY)
ENDIF(NOT ICET_FOUND)

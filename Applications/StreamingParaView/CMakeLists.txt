PROJECT(StreamingParaView)
SET(APP_NAME StreamingParaView)

SET(QT_USE_QTASSISTANT TRUE)
SET(QT_USE_QTNETWORK TRUE) # QtAssistant depends on it (only for linking)
SET(QT_USE_QTUITOOLS TRUE)
INCLUDE(${QT_USE_FILE})

add_subdirectory(Plugin)

IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

INCLUDE_DIRECTORIES(
  ${StreamingParaView_SOURCE_DIR}
  ${StreamingParaView_BINARY_DIR}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqCustomViewModules.h
  pqCustomDisplayPolicy.h
  pqStreamingMainWindowCore.h
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(CUSTOM_GUI_RESOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomWriters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomReaders.xml 
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomFilters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomSources.xml
 )

SET(QT_COMPONENTS_CUSTOM_GUI_RESOURCES_CONTENTS)
FOREACH(RESOURCE ${CUSTOM_GUI_RESOURCES})
  GET_FILENAME_COMPONENT(ALIAS ${RESOURCE} NAME)
  FILE(RELATIVE_PATH RESOURCE ${CMAKE_CURRENT_BINARY_DIR} "${RESOURCE}")
  FILE(TO_NATIVE_PATH "${RESOURCE}" RESOURCE)
  SET(QT_COMPONENTS_CUSTOM_GUI_RESOURCES_CONTENTS 
      "${QT_COMPONENTS_CUSTOM_GUI_RESOURCES_CONTENTS}  <file alias=\"${ALIAS}\">${RESOURCE}</file>\n")
ENDFOREACH(RESOURCE)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pqExtraResources.qrc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/pqExtraResources.qrc" @ONLY)

SET(UI_RESOURCES
  "${CMAKE_CURRENT_BINARY_DIR}/pqExtraResources.qrc"
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)

IF(WIN32)
  SET(EXE_ICON pqClient.rc)
ENDIF(WIN32)

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FORMS}
  ${EXE_ICON}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
)

IF(WIN32)
  LINK_DIRECTORIES(${QT_LIBRARY_DIR})
ENDIF(WIN32)

IF(Q_WS_MAC)
  SET(apple_bundle_sources "MacIcon.icns")
  SET_SOURCE_FILES_PROPERTIES(
    "MacIcon.icns"
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
    )
  SET(MACOSX_BUNDLE_ICON_FILE MacIcon.icns)
  SET(MAKE_BUNDLE MACOSX_BUNDLE)
ENDIF(Q_WS_MAC)

# Add main server executables.
IF(NOT Q_WS_MAC)
  SET(PV_EXE_LIST ${STREAMING_APP_NAME})
ENDIF(NOT Q_WS_MAC)
# Add Qt assistant to the list of shared lib. forwarded executables.
# If this is enabled, the build system will create an executable
# call assistant. This executable will set the appropriate ld library
# path and execute assistant-real. It is the installation's requirement
# to install assistant-real
SET(PV_EXE_LIST ${PV_EXE_LIST} assistant)

SET(MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}")
IF(NOT Q_WS_MAC)
  SET(PV_EXE_NAME ${APP_NAME}${PV_EXE_SUFFIX})
ELSE(NOT Q_WS_MAC)
  SET(PV_EXE_NAME ${APP_NAME})
ENDIF(NOT Q_WS_MAC)

ADD_EXECUTABLE(${PV_EXE_NAME} WIN32 ${MAKE_BUNDLE}  
  main.cxx
  ProcessModuleGUIHelper.cxx
  ProcessModuleGUIHelper.h
  pqCustomViewModules.cxx
  pqCustomDisplayPolicy.cxx
  pqStreamingMainWindowCore.cxx
  pqStreamingMainWindowCore.h

  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${EXE_ICON}
  ${apple_bundle_sources}
)

TARGET_LINK_LIBRARIES(${PV_EXE_NAME}
  pqComponents
  pqCore
  pqWidgets
  QtTesting
  ${QT_LIBRARIES}
)

IF(WIN32)
  TARGET_LINK_LIBRARIES(${PV_EXE_NAME} ${QT_QTMAIN_LIBRARY} )
ENDIF(WIN32)

IF(BUILD_TESTING)
  #PART1 - Tests that run tests inside the application
  SET(TEST_XML ${StreamingParaView_SOURCE_DIR}/Testing/XML)
  SET(TEST_PYTHON ${StreamingParaView_SOURCE_DIR}/Testing/Python)
  SET(TEST_IMAGE ${PARAVIEW_DATA_ROOT}/Baseline)

  #StreamingApplication depends upon the streaming plugin
  SET(DOLLAR "$")
  IF(CMAKE_CONFIGURATION_TYPES)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE}/${APP_NAME} --test-directory=${PARAVIEW_TEST_DIR})
    SET(RUN_PATH ${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE})
  ELSE(CMAKE_CONFIGURATION_TYPES)
    SET(RUN_PATH ${EXECUTABLE_OUTPUT_PATH})
    IF(Q_WS_MAC)
      SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/${APP_NAME}.app/Contents/MacOS/${APP_NAME} --test-directory=${PARAVIEW_TEST_DIR})
    ELSE(Q_WS_MAC)
      SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/${APP_NAME} --test-directory=${PARAVIEW_TEST_DIR})
    ENDIF(Q_WS_MAC)
  ENDIF(CMAKE_CONFIGURATION_TYPES)

  IF (QT_TESTING_WITH_PYTHON AND PARAVIEW_DATA_ROOT)
    SET(PLUGIN_TESTS
      TestStreamingApplication
      )
    
    FOREACH(test ${PLUGIN_TESTS})
      ADD_TEST(pqSClient${test}
        ${CMAKE_COMMAND} -E chdir ${RUN_PATH}
        ${TEST_BINARY} -dr --run-test=${TEST_PYTHON}/${test}.py
        --test-directory=${PARAVIEW_TEST_DIR}
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)
    ENDFOREACH(test)
  ENDIF (QT_TESTING_WITH_PYTHON AND PARAVIEW_DATA_ROOT)

  #PART2 - Tests that use a driver to orchestrate several executables (client/server etc)

ENDIF(BUILD_TESTING)
  
# Add shared link forwarding executables if necessary.
IF(PV_NEED_SHARED_FORWARD)
  FOREACH(pvexe ${PV_EXE_LIST})
    SET(PV_FORWARD_EXE ${pvexe}${PV_EXE_SUFFIX})
    CONFIGURE_FILE(
      ${ParaView_SOURCE_DIR}/Servers/Executables/pv-forward.c.in
      ${CMAKE_CURRENT_BINARY_DIR}/${pvexe}-forward.c
      @ONLY IMMEDIATE)
    ADD_EXECUTABLE(${pvexe} ${CMAKE_CURRENT_BINARY_DIR}/${pvexe}-forward.c)
    ADD_DEPENDENCIES(${pvexe} ${pvexe}${PV_EXE_SUFFIX})
    INSTALL(TARGETS ${pvexe} DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime)
  ENDFOREACH(pvexe)
ENDIF(PV_NEED_SHARED_FORWARD)

IF(NOT Q_WS_MAC)
  INSTALL(TARGETS ${PV_EXE_NAME}
    DESTINATION ${PV_EXE_INSTALL}
    COMPONENT Runtime)
ENDIF(NOT Q_WS_MAC)

SET (QTLIBLIST QTCORE QTGUI QTNETWORK QTXML QTTEST QTSQL)
# the variable and library names are not the same
IF (WIN32)
  SET (QTLIBLIST ${QTLIBLIST} QTASSISTANTCLIENT)
ELSE (WIN32)
  SET (QTLIBLIST ${QTLIBLIST} QTASSISTANT)
ENDIF (WIN32)

IF(NOT Q_WS_MAC)
  FOREACH(qtlib ${QTLIBLIST})
    IF (NOT WIN32)
      #INSTALL(FILES ${QT_${qtlib}_LIBRARY_RELEASE} DESTINATION ${PV_INSTALL_LIB_DIR})
      GET_FILENAME_COMPONENT(QT_LIB_DIR_tmp ${QT_${qtlib}_LIBRARY_RELEASE} PATH)
      GET_FILENAME_COMPONENT(QT_LIB_NAME_tmp ${QT_${qtlib}_LIBRARY_RELEASE} NAME)
      FILE(GLOB QT_LIB_LIST RELATIVE ${QT_LIB_DIR_tmp} "${QT_${qtlib}_LIBRARY_RELEASE}*")
      INSTALL(CODE "
MESSAGE(STATUS \"Installing \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR}/${QT_LIB_NAME_tmp}\")
EXECUTE_PROCESS (WORKING_DIRECTORY ${QT_LIB_DIR_tmp}
                 COMMAND tar c ${QT_LIB_LIST}
                 COMMAND tar -xC \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR})
        " COMPONENT Runtime)
    ELSE (NOT WIN32)
      GET_FILENAME_COMPONENT(QT_DLL_PATH_tmp ${QT_QMAKE_EXECUTABLE} PATH)
      INSTALL(FILES ${QT_DLL_PATH_tmp}/${qtlib}4.dll DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime)
    ENDIF (NOT WIN32)
    
  ENDFOREACH(qtlib)
ENDIF(NOT Q_WS_MAC)

# Hard-coded install rules for anything left to install. These should
# probably go to a local cmake file. Change to match the build system's
# libraries
# INSTALL(FILES /usr/lib/libstdc++.so.5 /lib/libgcc_s.so.1 /usr/lib/libpng.so.3 /usr/lib/libexpat.so.0 /usr/lib/libfontconfig.so.1 /usr/lib/libfreetype.so.6 /lib/libz.so.1 DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime)

# We also need Qt assistant
IF(WIN32)
  INSTALL(FILES ${QT_ASSISTANT_EXECUTABLE}
    DESTINATION ${PV_EXE_INSTALL} 
    PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
    COMPONENT Runtime
    RENAME assistant)
ELSE(WIN32)
  IF (NOT Q_WS_MAC)
    IF(PV_NEED_SHARED_FORWARD)
      INSTALL(FILES ${QT_ASSISTANT_EXECUTABLE}
        DESTINATION ${PV_EXE_INSTALL} 
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
        COMPONENT Runtime
        RENAME assistant-real)
    ELSE(PV_NEED_SHARED_FORWARD)
      INSTALL(FILES ${QT_ASSISTANT_EXECUTABLE}
        DESTINATION ${PV_EXE_INSTALL} 
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ 
        COMPONENT Runtime
        RENAME assistant)
    ENDIF(PV_NEED_SHARED_FORWARD)
  ENDIF (NOT Q_WS_MAC)
ENDIF(WIN32)

# Call this last so the install script runs after the above targets are installed:
#
IF(Q_WS_MAC)
  SET(OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT "${ParaView_BINARY_DIR}/${PV_EXE_NAME}_OSX_MakeStandAloneBundle.cmake")
  SET(OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT "${ParaView_BINARY_DIR}/${PV_EXE_NAME}_OSX_MakeStandAloneBundle.sh")

  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CompleteBundle.cmake.in"
    "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" @ONLY IMMEDIATE)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundle.sh.in"
    "${OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT}" @ONLY IMMEDIATE)

  INSTALL(SCRIPT "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}")
ENDIF(Q_WS_MAC)

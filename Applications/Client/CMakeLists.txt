PROJECT(pqClient)

MACRO(GET_IMAGE_THRESHOLD_ARG variable testname)
# Macro used to obtain the command argument to set the image threshold.
  SET (${variable})
  IF (${testname}_THRESHOLD)
    SET (${variable} --image-threshold=${${testname}_THRESHOLD})
  ENDIF (${testname}_THRESHOLD)
ENDMACRO(GET_IMAGE_THRESHOLD_ARG)

MACRO(GET_REVERSE_CONNECT_ARG variable testname)
# Macro used to obtain when this test must use reverse connection
  SET (${variable})
  IF (${testname}_REVERSE_CONNECT)
    SET (${variable} --test-rc)
  ENDIF (${testname}_REVERSE_CONNECT)
ENDMACRO(GET_REVERSE_CONNECT_ARG)

SET(QT_USE_QTUITOOLS TRUE)
INCLUDE(${QT_USE_FILE})

IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

INCLUDE_DIRECTORIES(
  ${pqClient_SOURCE_DIR}
  ${pqClient_BINARY_DIR}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

IF(WIN32)
  SET(EXE_ICON pqClient.rc)
ENDIF(WIN32)

SOURCE_GROUP("Resources" FILES
  ${EXE_ICON}
)

IF(WIN32)
  LINK_DIRECTORIES(${QT_LIBRARY_DIR})
ENDIF(WIN32)

IF(Q_WS_MAC)
  SET(apple_bundle_sources
    "MacIcon.icns"
    "${CMAKE_SOURCE_DIR}/Common/DocumentIcons/OpenFOAM.icns"
    "${CMAKE_SOURCE_DIR}/Common/DocumentIcons/vtk.icns"
    )
  SET_SOURCE_FILES_PROPERTIES(
    ${apple_bundle_sources}
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
    )
  IF(QT_MAC_USE_COCOA)
    GET_FILENAME_COMPONENT(qt_menu_nib
      "@QT_QTGUI_LIBRARY_RELEASE@/Resources/qt_menu.nib"
      REALPATH)
    set(qt_menu_nib_sources
      "${qt_menu_nib}/classes.nib"
      "${qt_menu_nib}/info.nib"
      "${qt_menu_nib}/keyedobjects.nib"
      )
    SET_SOURCE_FILES_PROPERTIES(
      ${qt_menu_nib_sources}
      PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources/qt_menu.nib
      )
  ELSE(QT_MAC_USE_COCOA)
    set(qt_menu_nib_sources)
  ENDIF(QT_MAC_USE_COCOA)
  SET(MACOSX_BUNDLE_ICON_FILE MacIcon.icns)
  SET(MAKE_BUNDLE MACOSX_BUNDLE)
ENDIF(Q_WS_MAC)

# Add main server executables.
IF(NOT Q_WS_MAC)
  SET(PV_EXE_LIST paraview)
ENDIF(NOT Q_WS_MAC)

SET(MACOSX_BUNDLE_BUNDLE_NAME "ParaView")
IF(NOT Q_WS_MAC)
  SET(PV_EXE_NAME paraview${PV_EXE_SUFFIX})
ELSE(NOT Q_WS_MAC)
  SET(PV_EXE_NAME paraview)
ENDIF(NOT Q_WS_MAC)

ADD_EXECUTABLE(${PV_EXE_NAME} WIN32 ${MAKE_BUNDLE}  
  main.cxx
  ProcessModuleGUIHelper.cxx
  ProcessModuleGUIHelper.h
  ${EXE_ICON}
  ${apple_bundle_sources}
  ${qt_menu_nib_sources}
)

# Need an extended Info.plist to register all the file types on Mac OS X
# Further also fill it with version and copyright info.
# Michael Wild <wild@ifd.mavt.ethz.ch>
SET_TARGET_PROPERTIES(${PV_EXE_NAME} PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in
  MACOSX_BUNDLE_GUI_IDENTIFIER com.kitware.paraview
  MACOSX_BUNDLE_LONG_VERSION_STRING ${PARAVIEW_VERSION_FULL}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PARAVIEW_VERSION}
  MACOSX_BUNDLE_BUNDLE_VERSION ${PARAVIEW_VERSION_FULL}
  MACOSX_BUNDLE_INFO_STRING "ParaView visualization tool by Kitware Inc."
  MACOSX_BUNDLE_COPYRIGHT "© 2000-2009, Kitware Inc., Sandia National Laboratories, Los Alamos National Laboratory, Army Research Laboratory, CSimSoft"
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
  IF(Q_WS_MAC)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS/paraview --test-directory=${PARAVIEW_TEST_DIR})
  ELSE(Q_WS_MAC)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview --test-directory=${PARAVIEW_TEST_DIR})
  ENDIF(Q_WS_MAC)

  SET(TEST_XML ${pqClient_SOURCE_DIR}/Testing/XML)
  SET(TEST_PYTHON ${pqClient_SOURCE_DIR}/Testing/Python)
  SET(TEST_IMAGE ${PARAVIEW_DATA_ROOT}/Baseline)

  SET (XML_TESTS_WITH_BASELINES
    # ColorEditor3 doesn't work in client/server mode
    ColorEditor3
#    LoadStateSelection
    OpenFile
#    SelectionUndo
    )

  SET (TESTS_WITHOUT_BASELINES
    AboutDialog
    ChartOptions
    )
  IF(PARAVIEW_DATA_ROOT)
    SET (TESTS_WITHOUT_BASELINES
      ${TESTS_WITHOUT_BASELINES}
# FIXME: Enable after Plots are converted to new style and backwards
# compatibility issues are fixed.
#LoadStateHistogram
      )
  ENDIF(PARAVIEW_DATA_ROOT)

  IF(QT_TESTING_WITH_PYTHON)
    SET(XML_TESTS_WITH_BASELINES ${XML_TESTS_WITH_BASELINES}
      PythonProgrammableFilter
      )
    SET(PYTHON_TESTS
      OpenHelp
      ToolBarAndMenus
      TestContourWidget
      )
    IF(PARAVIEW_DATA_ROOT)
      SET(PYTHON_TESTS ${PYTHON_TESTS}
        #HistogramChart
        LoadStateMultiView
        FileSeries
        SaveLargeScreenshot
        #SaveAnimationMultiView   -- need to fix race condition on Mac
        )
      # Set image threshold overrides for the tests.
      SET(SaveLargeScreenshot_THRESHOLD 10000)

      SET(PYTHON_TESTS_REMOTE_ONLY 
        DisconnectAndSaveAnimation)
    ENDIF(PARAVIEW_DATA_ROOT)
  ENDIF(QT_TESTING_WITH_PYTHON)

  FOREACH(test ${TESTS_WITHOUT_BASELINES})
    ADD_TEST(pqClient${test}
      ${TEST_BINARY} -dr 
        --test-directory=${PARAVIEW_TEST_DIR}
        --run-test=${TEST_XML}/${test}.xml --exit)
  ENDFOREACH(test)
  
  IF(PARAVIEW_DATA_ROOT)
    FOREACH(test ${XML_TESTS_WITH_BASELINES})
      GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
      ADD_TEST(pqClient${test}
        ${TEST_BINARY} -dr --run-test=${TEST_XML}/${test}.xml
        --test-directory=${PARAVIEW_TEST_DIR}
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)
    ENDFOREACH(test) 
  ENDIF(PARAVIEW_DATA_ROOT)
  
  FOREACH(test ${PYTHON_TESTS})
    GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
    ADD_TEST(pqClient${test}-Python
      ${TEST_BINARY} -dr --run-test=${TEST_PYTHON}/${test}.py
      --test-directory=${PARAVIEW_TEST_DIR}
      ${THRESHOLD_CMD}
      --exit)
  ENDFOREACH(test)

  # test the plugins
  IF(BUILD_SHARED_LIBS AND BUILD_EXAMPLES)
    SET(DOLLAR "$")
    IF(CMAKE_CONFIGURATION_TYPES)
      SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE}/paraview --test-directory=${PARAVIEW_TEST_DIR})
      SET(RUN_PATH ${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE})
    ELSE(CMAKE_CONFIGURATION_TYPES)
      SET(RUN_PATH ${EXECUTABLE_OUTPUT_PATH})
      IF(Q_WS_MAC)
        SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS/paraview --test-directory=${PARAVIEW_TEST_DIR})
      ELSE(Q_WS_MAC)
        SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview --test-directory=${PARAVIEW_TEST_DIR})
      ENDIF(Q_WS_MAC)
    ENDIF(CMAKE_CONFIGURATION_TYPES)
  

    IF (QT_TESTING_WITH_PYTHON)
      # Since these tests need python, it's essential to verify that testing
      # with python is enabled.
      SET(PLUGIN_TESTS
          ToolBarPlugin
          ObjectPanelPlugin
          )
      IF (PARAVIEW_BUILD_PLUGIN_pvblot)
        SET(PLUGIN_TESTS ${PLUGIN_TESTS} PVBlotTest1)
      ENDIF (PARAVIEW_BUILD_PLUGIN_pvblot)

      FOREACH(test ${PLUGIN_TESTS})
        ADD_TEST(pqClient${test}
                 ${CMAKE_COMMAND} -E chdir ${RUN_PATH}
                 ${TEST_BINARY} -dr --run-test=${TEST_PYTHON}/${test}.py
                 --exit)
      ENDFOREACH(test)
    ENDIF (QT_TESTING_WITH_PYTHON)

  ENDIF(BUILD_SHARED_LIBS AND BUILD_EXAMPLES)


# Now we add tests that use the pvTestDriver for running.
  SET(XML_TESTS_WITH_BASELINES
    3DWidgetInCustomFilter
    AnimatePipelineTime
    CalcParens
    Calculator
    CameraLink
    Clip
    ColorEditor1
    ColorEditor2
    ColorEditor4
    Contour
    ContourRange
#    CreateAndLoadLookmark -- disabling since it fails with Qt 4.5 
                  # and we are going to deprecate lookmarks anyways.
    CustomFilter
    CustomSourceProbe
#    CutMulti -Randomly failing test.
    D3SmallCells
    DualSphereAnimation
    ExodusIIPanel
    ExodusModeShapes
#    ExodusWriter -- ENABLE WHEN WRITER IS ADDED
    ExodusXML
    ExTimeseries
    ExTimeseries2
    ExtractBlock
    ExtractLevel
    FFTOverTime
    Flow
    Flow2
    Fractal2D
    Fractal3D
    GlobalProperties
    LinePlot
    LoadSaveStateAnimation
    LoadState
    NormalGlyphs
    ObjectInspectorSphere
    OpenSaveData
    Plot3DReader
    PlotEdges
    PlotEdges2
    PropertyLink
    ReadXMLPolyDataFileSeries
    RectilinearFractal
    RepresentationSelector
    SaveAnimationGeometry
    SaveColorMap
    SaveCSV
    SelectReader
    #Selection
    SimpleInteraction
    Slice
    SPTimeseries
    StreamTracerUpdates
    TemporalInterpolator
    TemporalShiftScale
    Tessellate
    Threshold
    UndoRedo
    UndoRedo1
    UndoRedo2
    UndoRedo3
    #UndoRedo4 -- disabled since AnimationPanel was removed. We need to create a
               #new test to test animation undo-redo using the animation view.
    UndoRedo5
    UndoRedo6
    UndoRedo7
    UnstructuredOutline
    VariableSelector
    ViewSettingsDialog
    XdmfRead
    XdmfReadImageData
    XdmfReadImageDataCollection
    XdmfReadRectilinearGrid
    XdmfReadRectilinearGridCollection
    XdmfReadStructuredGrid
    XdmfReadStructuredGridCollection
    XdmfGridAttributes
  )

  # Add image threshold overrides for tests.
  # Generally all tests with wireframes need higher thresholds.
  SET (Flow2_THRESHOLD 15)
  SET (UndoRedo4_THRESHOLD 15)

  # These Xdmf tests have wireframes, hence the increased thresholds.
  SET (XdmfReadImageData_THRESHOLD 20)
  SET (XdmfReadRectilinearGrid_THRESHOLD 20)
  SET (XdmfReadRectilinearGridCollection_THRESHOLD 20)
  SET (XdmfReadStructuredGrid_THRESHOLD 20)
  SET (XdmfReadStructuredGridCollection_THRESHOLD 20)
  SET (XdmfReadImageDataCollection_THRESHOLD 20)

  # This test renders points and they are offsetted a bit differently on
  # different platforms so just increase the threshold
  SET (SaveCSV_THRESHOLD 40)

  # Disable some testing configurations for these tests.
  ## Disable ClientRenderServer tests for FFTOverTime. This is done since
  ## selection is not supported in render server mode esp. when number of render
  ## server processess is not same as the data server processes
  SET (FFTOverTime_DISABLE_CRS TRUE)

  # Make these tests use reverse connection.
  SET (CutMulti_REVERSE_CONNECT TRUE)

  # Test compositing for a few client server and client render server tests.
  SET(COMPOSITING_XML_TESTS_WITH_BASELINES
    Calculator
    CameraLink
    ColorEditor1
    ContourRange
    D3SmallCells
    Fractal3D
    StreamTracerUpdates
    UndoRedo2
    )

  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/pvTestDriver)

  IF(PARAVIEW_DATA_ROOT)
    OPTION(PARAVIEW_CLIENT_RENDER_SERVER_TESTS
      "Turn on/off client tests that involve separate data and render servers."
      ON)
    MARK_AS_ADVANCED(PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    FOREACH(test ${XML_TESTS_WITH_BASELINES})
      GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
      GET_REVERSE_CONNECT_ARG(REVERSE_CONNECTION_CMD ${test})

      ADD_TEST(pqClient${test}-Builtin
        ${TEST_BINARY}
        --test-directory=${PARAVIEW_TEST_DIR}
        -dr
        --run-test=${TEST_XML}/${test}.xml
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)
      
      ADD_TEST(pqClient${test}-ClientServer
        ${TEST_BINARY} --test-server
        ${REVERSE_CONNECTION_CMD}
        --test-directory=${PARAVIEW_TEST_DIR}
        -dr
        --run-test=${TEST_XML}/${test}.xml
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)

      IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
        IF (NOT ${test}_DISABLE_CRS)
          ADD_TEST(pqClient${test}-ClientRenderServer
            ${TEST_BINARY} --test-render-server
            ${REVERSE_CONNECTION_CMD}
            --test-directory=${PARAVIEW_TEST_DIR}
            -dr
            --run-test=${TEST_XML}/${test}.xml
            --compare-view=${TEST_IMAGE}/${test}.png
            ${THRESHOLD_CMD}
            --exit)
        ENDIF (NOT ${test}_DISABLE_CRS)
      ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    ENDFOREACH(test)

    FOREACH(test ${COMPOSITING_XML_TESTS_WITH_BASELINES})
      GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
      ADD_TEST(pqClient${test}-ClientServerRemoteRender
        ${TEST_BINARY} --test-server --test-remote-rendering
        --test-directory=${PARAVIEW_TEST_DIR}
        -dr
        --run-test=${TEST_XML}/${test}.xml
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)
        
      IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
        ADD_TEST(pqClient${test}-ClientRenderServerRemoteRender
          ${TEST_BINARY} --test-render-server --test-remote-rendering
          --test-directory=${PARAVIEW_TEST_DIR}
          -dr
          --run-test=${TEST_XML}/${test}.xml
          --compare-view=${TEST_IMAGE}/${test}.png
          ${THRESHOLD_CMD}
          --exit)
      ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    ENDFOREACH(test)

    FOREACH(test ${PYTHON_TESTS_REMOTE_ONLY})
        GET_IMAGE_THRESHOLD_ARG(THRESHOLD_CMD ${test})
        ADD_TEST(pqClient${test}-ClientServer-Python
          ${TEST_BINARY} --test-server
          --test-directory=${PARAVIEW_TEST_DIR}
          -dr
          --run-test=${TEST_PYTHON}/${test}.py
          ${THRESHOLD_CMD}
          --exit)

     #FIXME
     #IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
     #  ADD_TEST(pqClient${test}-ClientRenderServer-Python
     #    ${TEST_BINARY} --test-render-server
     #    --test-directory=${PARAVIEW_TEST_DIR}
     #    -dr
     #    --run-test=${TEST_PYTHON}/${test}.py
     #    --exit)
     #ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    ENDFOREACH(test) 

    ADD_TEST(StructuredVolumeRender-ClientServerRemoteRender
      ${TEST_BINARY} --test-server --test-remote-rendering
      --test-directory=${PARAVIEW_TEST_DIR}
      -dr
      --run-test=${TEST_XML}/StructuredVolumeRender.xml
      --compare-view=${TEST_IMAGE}/StructuredVolumeRender.png
      --exit)
    ADD_TEST(UnstructuredVolumeRender-ClientServerRemoteRender
      ${TEST_BINARY} --test-server --test-remote-rendering
      --test-directory=${PARAVIEW_TEST_DIR}
      -dr
      --run-test=${TEST_XML}/UnstructuredVolumeRender.xml
      --compare-view=${TEST_IMAGE}/UnstructuredVolumeRender.png
      --exit)

  ENDIF(PARAVIEW_DATA_ROOT)

  ADD_SUBDIRECTORY(Testing/Cxx)

  # Do the link forwarding for the test driver as well.
  SET(PV_EXE_LIST ${PV_EXE_LIST} pvTestDriver)
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

# Install system libraries on Linux (will not work on any other Unix
# yet)
FOREACH(fmpglib avcodec avformat avutil)
  IF(FFMPEG_${fmpglib}_LIBRARY)
    GET_FILENAME_COMPONENT(FFMPEG_LIB_DIR_tmp ${FFMPEG_${fmpglib}_LIBRARY} PATH)
    GET_FILENAME_COMPONENT(FFMPEG_LIB_NAME_tmp ${FFMPEG_${fmpglib}_LIBRARY} NAME)
    FILE(GLOB FFMPEG_LIB_LIST RELATIVE ${FFMPEG_LIB_DIR_tmp} "${FFMPEG_${fmpglib}_LIBRARY}*")
    INSTALL(CODE "
MESSAGE(STATUS \"Installing \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR}/${FFMPEG_LIB_NAME_tmp}\")
EXECUTE_PROCESS (WORKING_DIRECTORY ${FFMPEG_LIB_DIR_tmp}
                 COMMAND tar c ${FFMPEG_LIB_LIST}
                 COMMAND tar -xC \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR})
        " COMPONENT Runtime)
    #INSTALL(FILES ${FFMPEG_${fmpglib}_LIBRARY} DESTINATION ${PV_INSTALL_LIB_DIR})
    
  ENDIF(FFMPEG_${fmpglib}_LIBRARY)
ENDFOREACH(fmpglib)

SET (QTLIBLIST QTCORE QTGUI QTNETWORK QTXML QTTEST QTSQL QTOPENGL QTHELP QTCLUCENE)

IF (VTK_USE_QVTK_QTOPENGL)
  SET (QTLIBLIST ${QTLIBLIST} QTOPENGL)
ENDIF (VTK_USE_QVTK_QTOPENGL)

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

  # Now install the plugins for sqldrivers.
  INSTALL(DIRECTORY ${QT_PLUGINS_DIR}/sqldrivers 
          DESTINATION ${PV_INSTALL_LIB_DIR}
          COMPONENT Runtime
          FILES_MATCHING
            PATTERN "*.so"
            PATTERN "*.dll")
ENDIF(NOT Q_WS_MAC)

# Hard-coded install rules for anything left to install. These should
# probably go to a local cmake file. Change to match the build system's
# libraries
# INSTALL(FILES /usr/lib/libstdc++.so.5 /lib/libgcc_s.so.1 /usr/lib/libpng.so.3 /usr/lib/libexpat.so.0 /usr/lib/libfontconfig.so.1 /usr/lib/libfreetype.so.6 /lib/libz.so.1 DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime)


# Call this last so the install script runs after the above targets are installed:
#
IF(Q_WS_MAC)
  SET(OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT "${ParaView_BINARY_DIR}/${PV_EXE_NAME}_OSX_MakeStandAloneBundle.cmake")
  SET(OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT "${ParaView_BINARY_DIR}/${PV_EXE_NAME}_OSX_MakeStandAloneBundle.sh")

  SET (QT_SQLLITE_PLUGINS_DIR ${QT_PLUGINS_DIR}/sqldrivers)

  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CompleteBundle.cmake.in"
    "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" @ONLY IMMEDIATE)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundle.sh.in"
    "${OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT}" @ONLY IMMEDIATE)

  INSTALL(SCRIPT "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" COMPONENT Bundle)
ENDIF(Q_WS_MAC)

#########################################################################
# Create a drag and drop bundle on Mac OS X
if(Q_WS_MAC)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/CreateDragNDropBundle.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/CreateDragNDropBundle.sh @ONLY)
  # read the license file
  FILE(READ ${CMAKE_CURRENT_SOURCE_DIR}/DmgLicense_v1.2.txt LICENSE_TEXT)
  # escape dangerous characters (that's just ridiculous...)
  STRING(REGEX REPLACE "([\\\"])" "\\\\\\1" LICENSE_TEXT "${LICENSE_TEXT}")
  # wrap lines in quotes and end them with \n
  STRING(REPLACE "\n" "\\n\"\n\"" LICENSE_TEXT "${LICENSE_TEXT}")
  SET(LICENSE_TEXT "\"${LICENSE_TEXT}\\n\"\n")
  # configure the resource file
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/DmgResource.r.in
    ${CMAKE_CURRENT_BINARY_DIR}/DmgResource.r @ONLY)
  # now define a target to run the thing
  ADD_CUSTOM_TARGET(dmg
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/CreateDragNDropBundle.sh all
    DEPENDS ${PV_EXE_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating drag-n-drop installer disk image"
    VERBATIM
    )
endif(Q_WS_MAC)

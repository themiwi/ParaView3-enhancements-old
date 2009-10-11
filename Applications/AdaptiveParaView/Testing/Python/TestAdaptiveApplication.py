#/usr/bin/env python

import QtTesting
import sys

lib1name = 'libAdaptiveSSPlugin.so'
lib2name = 'libAdaptiveCSPlugin.so'
if sys.platform == 'win32':
    lib1name = 'AdaptiveSSPlugin.dll'
    lib2name = 'AdaptiveCSPlugin.dll'
if sys.platform == 'darwin':
    lib1name = 'libAdaptiveSSPlugin.dylib'
    lib2name = 'libAdaptiveCSPlugin.dylib'

object1 = 'pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(object1, 'activate', 'actionManage_Plugins')
object2 = "pqClientMainWindow/pqPluginDialog/localGroup/loadLocal"
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/pqPluginDialog/pqFileDialog'
QtTesting.playCommand(object3, 'filesSelected', lib1name)
object4 = 'pqClientMainWindow/pqPluginDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'actionManage_Plugins')
QtTesting.playCommand(object2, 'activate', '')
object5 = 'pqClientMainWindow/pqPluginDialog/pqFileDialog'
QtTesting.playCommand(object3, 'filesSelected', lib2name)
QtTesting.playCommand(object4, 'activate', '')
object6 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/CloseButton'
QtTesting.playCommand(object6, 'activate', '')
object7 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/1QWidget0/1QScrollArea0/qt_scrollarea_viewport/EmptyView/ConvertActionsFrame/1QPushButton0'
QtTesting.playCommand(object7, 'activate', '')
object8='pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(object8, 'activate', 'actionTesting_Window_Size')

object9 = 'pqClientMainWindow/menubar/menuFile'
QtTesting.playCommand(object9, 'activate', 'actionFileOpen')
object9a = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object9a, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/noisy_10x100x100_sphere.raw')

object9b = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/WholeExtent_1'
QtTesting.playCommand(object9b, 'set_string', '10')
object9b = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/WholeExtent_3'
QtTesting.playCommand(object9b, 'set_string', '100')
object9c = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/WholeExtent_5'
QtTesting.playCommand(object9c, 'set_string', '100')
object9d = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object9d, 'activate', '')

object10 = 'pqClientMainWindow/menubar/menuFilters/Alphabetical'
QtTesting.playCommand(object10, 'activate', 'Contour')
object10b = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object10b, 'activate', '')


object11 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser/PipelineView'
QtTesting.playCommand(object11, 'mousePress', '1,1,0,13,10,/0:0/0:0/0:1')
QtTesting.playCommand(object11, 'mouseRelease', '1,0,0,13,10,/0:0/0:0/0:1')

object12 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTesting.playCommand(object12, 'mousePress', '(0.627119,0.617371,1,1,0)')
QtTesting.playCommand(object12, 'mouseMove', '(0.510358,0.483568,1,0,0)')
QtTesting.playCommand(object12, 'mouseRelease', '(0.510358,0.483568,1,0,0)')

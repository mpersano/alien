project(Gui)

set(Gui_SOURCES
    AbstractItem.cpp
    AbstractItem.h
    AbstractWorldController.cpp
    AbstractWorldController.h
    ActionController.cpp
    ActionController.h
    ActionHolder.cpp
    ActionHolder.h
    ActionModel.cpp
    ActionModel.h
    BugReportController.cpp
    BugReportController.h
    BugReportLogger.cpp
    BugReportLogger.h
    BugReportView.cpp
    BugReportView.h
    CellComputerEditTab.cpp
    CellComputerEditTab.h
    CellConnectionItem.cpp
    CellConnectionItem.h
    CellEditTab.cpp
    CellEditTab.h
    CellItem.cpp
    CellItem.h
    ClusterEditTab.cpp
    ClusterEditTab.h
    CodeEditWidget.cpp
    CodeEditWidget.h
    ColorizeDialogController.cpp
    ColorizeDialogController.h
    ComputationSettingsDialog.cpp
    ComputationSettingsDialog.h
    ComputationSettingsWidget.cpp
    ComputationSettingsWidget.h
    CoordinateSystem.cpp
    CoordinateSystem.h
    DataAnalyzer.cpp
    DataAnalyzer.h
    DataEditContext.cpp
    DataEditContext.h
    DataEditController.cpp
    DataEditController.h
    DataEditModel.cpp
    DataEditModel.h
    DataEditView.cpp
    DataEditView.h
    DataRepository.cpp
    DataRepository.h
    Definitions.h
    FileLogger.cpp
    FileLogger.h
    GeneralInfoController.cpp
    GeneralInfoController.h
    GettingStartedWindow.cpp
    GettingStartedWindow.h
    GridMultiplierDialog.cpp
    GridMultiplierDialog.h
    GuiLogger.cpp
    GuiLogger.h
    HexEditWidget.cpp
    HexEditWidget.h
    ItemConfig.cpp
    ItemConfig.h
    ItemManager.cpp
    ItemManager.h
    ItemViewport.cpp
    ItemViewport.h
    ItemWorldController.cpp
    ItemWorldController.h
    Jobs.h
    LoggingController.cpp
    LoggingController.h
    LoggingView.cpp
    LoggingView.h
    MainController.cpp
    MainController.h
    Main.cpp
    MainModel.cpp
    MainModel.h
    MainView.cpp
    MainView.h
    MarkerItem.cpp
    MarkerItem.h
    MetadataEditTab.cpp
    MetadataEditTab.h
    MetadataEditWidget.cpp
    MetadataEditWidget.h
    MonitorController.cpp
    MonitorController.h
    MonitorView.cpp
    MonitorView.h
    NewDiscDialog.cpp
    NewDiscDialog.h
    NewHexagonDialog.cpp
    NewHexagonDialog.h
    NewParticlesDialog.cpp
    NewParticlesDialog.h
    NewRectangleDialog.cpp
    NewRectangleDialog.h
    NewSimulationDialog.cpp
    NewSimulationDialog.h
    Notifier.cpp
    Notifier.h
    OpenGLWorldController.cpp
    OpenGLWorldController.h
    OpenGLWorldScene.cpp
    OpenGLWorldScene.h
    ParticleEditTab.cpp
    ParticleEditTab.h
    ParticleItem.cpp
    ParticleItem.h
    ProgressBar.cpp
    ProgressBar.h
    QApplicationHelper.h
    Queue.cpp
    Queue.h
    RandomMultiplierDialog.cpp
    RandomMultiplierDialog.h
    SelectionEditTab.cpp
    SelectionEditTab.h
    SendLastImageJob.cpp
    SendLastImageJob.h
    SendLiveImageJob.cpp
    SendLiveImageJob.h
    SendStatisticsJob.cpp
    SendStatisticsJob.h
    Settings.cpp
    Settings.h
    SimulationConfig.cpp
    SimulationConfig.h
    SimulationParametersDialog.cpp
    SimulationParametersDialog.h
    SimulationViewController.cpp
    SimulationViewController.h
    SimulationViewSettings.h
    SimulationViewWidget.cpp
    SimulationViewWidget.h
    SnapshotController.cpp
    SnapshotController.h
    StartupController.cpp
    StartupController.h
    StringHelper.h
    SymbolEditTab.cpp
    SymbolEditTab.h
    SymbolTableDialog.cpp
    SymbolTableDialog.h
    TabWidgetHelper.h
    TokenEditTab.cpp
    TokenEditTab.h
    TokenEditTabWidget.cpp
    TokenEditTabWidget.h
    TokenEditWidget.cpp
    TokenEditWidget.h
    ToolbarContext.cpp
    ToolbarContext.h
    ToolbarController.cpp
    ToolbarController.h
    ToolbarView.cpp
    ToolbarView.h
    ViewportInterface.cpp
    ViewportInterface.h
    WebSimulationController.cpp
    WebSimulationController.h
    WebSimulationSelectionController.cpp
    WebSimulationSelectionController.h
    WebSimulationSelectionView.cpp
    WebSimulationSelectionView.h
    WebSimulationTableModel.cpp
    WebSimulationTableModel.h
    ZoomActionController.cpp
    ZoomActionController.h
    Resources/resources.qrc
)

set(Gui_LINK_LIBRARIES
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Boost::serialization
    QJsonModel
    ALiEn::Base
    ALiEn::EngineInterface
    ALiEn::EngineGpu
    ALiEn::EngineGpuKernels
    ALiEn::Web
)

add_executable(Gui ${Gui_SOURCES})

target_link_libraries(Gui PUBLIC ${Gui_LINK_LIBRARIES})

target_include_directories(Gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

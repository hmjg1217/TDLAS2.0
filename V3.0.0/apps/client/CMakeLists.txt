add_executable(tdlas_client_codex_template WIN32
    src/main.cpp
    src/ClientTemplateWindow.cpp
    src/ClientTemplateWindow.h
    src/ChartWidget.cpp
    src/ChartWidget.h
    src/CameraPreviewWidget.cpp
    src/CameraPreviewWidget.h
    resources/client.qrc
    ../../shared/resources/sidebar-icons.qrc
)

tdlas_configure_ui_target(tdlas_client_codex_template)
install(TARGETS tdlas_client_codex_template RUNTIME DESTINATION .)

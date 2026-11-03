add_executable(tdlas_admin_codex_template WIN32
    src/main.cpp
    src/AdminTemplateWindow.cpp
    src/AdminTemplateWindow.h
    src/ChartWidget.cpp
    src/ChartWidget.h
    src/CameraPreviewWidget.cpp
    src/CameraPreviewWidget.h
    resources/admin.qrc
    ../../shared/resources/sidebar-icons.qrc
)

tdlas_configure_ui_target(tdlas_admin_codex_template)
install(TARGETS tdlas_admin_codex_template RUNTIME DESTINATION .)

add_executable(tdlas_startup_codex_template WIN32
    src/main.cpp
    src/StartupTemplateWindow.cpp
    src/StartupTemplateWindow.h
    resources/startup.qrc
)

tdlas_configure_ui_target(tdlas_startup_codex_template)
install(TARGETS tdlas_startup_codex_template RUNTIME DESTINATION .)

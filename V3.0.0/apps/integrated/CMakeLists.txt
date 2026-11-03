set(TDLAS_INTEGRATED_RESOURCES
    ../startup/resources/startup.qrc
    ../admin/resources/admin.qrc
    ../client/resources/client.qrc
    ../../shared/resources/sidebar-icons.qrc
)

add_library(tdlas_integrated_core STATIC
    src/IntegratedApplicationController.cpp
    src/IntegratedApplicationController.h
    src/TemporaryCredentialAuthenticator.cpp
    src/TemporaryCredentialAuthenticator.h
    ../startup/src/StartupTemplateWindow.cpp
    ../startup/src/StartupTemplateWindow.h
    ../admin/src/AdminTemplateWindow.cpp
    ../admin/src/AdminTemplateWindow.h
    ../admin/src/ChartWidget.cpp
    ../admin/src/ChartWidget.h
    ../admin/src/CameraPreviewWidget.cpp
    ../admin/src/CameraPreviewWidget.h
    ../client/src/ClientTemplateWindow.cpp
    ../client/src/ClientTemplateWindow.h
    ../client/src/ChartWidget.cpp
    ../client/src/ChartWidget.h
    ../client/src/CameraPreviewWidget.cpp
    ../client/src/CameraPreviewWidget.h
)

target_include_directories(tdlas_integrated_core
    PUBLIC
        src
        ../startup/src
        ../admin/src
        ../client/src
)

tdlas_configure_ui_target(tdlas_integrated_core)

add_executable(tdlas_hydrogen_ui WIN32
    src/main.cpp
    ${TDLAS_INTEGRATED_RESOURCES}
)
target_link_libraries(tdlas_hydrogen_ui PRIVATE tdlas_integrated_core)
tdlas_configure_ui_target(tdlas_hydrogen_ui)
install(TARGETS tdlas_hydrogen_ui RUNTIME DESTINATION .)

if(BUILD_TESTING)
    add_executable(tdlas_authentication_test
        tests/TemporaryCredentialAuthenticatorTest.cpp
        src/TemporaryCredentialAuthenticator.cpp
        src/TemporaryCredentialAuthenticator.h
    )
    target_include_directories(tdlas_authentication_test PRIVATE src)
    target_link_libraries(tdlas_authentication_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
    add_test(
        NAME tdlas_authentication_routes
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "PATH=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
            "$<TARGET_FILE:tdlas_authentication_test>"
    )

    add_executable(tdlas_login_routing_test
        tests/IntegratedApplicationControllerTest.cpp
        ../startup/resources/startup.qrc
        ../admin/resources/admin.qrc
        ../client/resources/client.qrc
    )
    target_link_libraries(tdlas_login_routing_test PRIVATE tdlas_integrated_core)
    tdlas_configure_ui_target(tdlas_login_routing_test)
    add_test(
        NAME tdlas_login_window_routes
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
            "$<TARGET_FILE:tdlas_login_routing_test>"
    )
endif()

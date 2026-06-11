find_package(Threads REQUIRED)

add_executable(news_server
    src/main.cpp
    src/Auth.cpp
    src/Sessions.cpp
    src/Html.cpp
    src/Router.cpp
)

target_include_directories(news_server PRIVATE src)

target_link_libraries(news_server
    PRIVATE
        news_domain
        httplib::httplib
        Threads::Threads)

target_compile_features(news_server PRIVATE cxx_std_17)

add_custom_target(run
    COMMAND $<TARGET_FILE:news_server>
    DEPENDS news_server
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "news_server -> http://127.0.0.1:8080")

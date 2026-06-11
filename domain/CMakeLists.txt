add_library(news_domain STATIC
    src/DateTime.cpp
    src/User.cpp
    src/Author.cpp
    src/Category.cpp
    src/Tag.cpp
    src/News.cpp
    src/SearchCriteria.cpp
    src/NewsRepository.cpp
)

target_include_directories(news_domain
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(news_domain PUBLIC cxx_std_17)

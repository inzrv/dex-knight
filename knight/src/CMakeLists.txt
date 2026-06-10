file(GLOB_RECURSE KNIGHT_CORE_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

list(REMOVE_ITEM KNIGHT_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(knight_core ${KNIGHT_CORE_SOURCES})

target_include_directories(knight_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(knight_core
    PUBLIC
        Boost::boost
        spdlog::spdlog
        solabi::solabi
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
)

if(APPLE)
    target_compile_options(knight_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(knight main.cpp)
target_link_libraries(knight PRIVATE knight_core)

if(APPLE)
    target_compile_options(knight PRIVATE -Wall -Wextra -Wpedantic)
endif()

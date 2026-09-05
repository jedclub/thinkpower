cmake_minimum_required(VERSION 3.16)
project(ThinkPower VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED
    ayatana-appindicator3-0.1
    gtk+-3.0
    gio-2.0
)

add_executable(power-tray src/power-tray.cpp)

target_include_directories(power-tray PRIVATE ${DEPS_INCLUDE_DIRS})
target_compile_options(power-tray PRIVATE ${DEPS_CFLAGS_OTHER} -Wall -Wextra -Wno-deprecated-declarations -O3)
target_link_libraries(power-tray PRIVATE ${DEPS_LIBRARIES})

install(TARGETS power-tray DESTINATION bin)
install(PROGRAMS src/power-profile-manager DESTINATION bin)

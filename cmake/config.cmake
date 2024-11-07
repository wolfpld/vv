file(GENERATE OUTPUT .gitignore CONTENT "*")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(MOLD_LINKER mold)
    if(MOLD_LINKER)
        set(CMAKE_LINKER_TYPE "MOLD")
    endif()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-fno-eliminate-unused-debug-types)
    endif()
endif()

set(CMAKE_COLOR_DIAGNOSTICS ON)

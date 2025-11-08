if(NOT TARGET dep::ac)
    add_library(dep_ac INTERFACE IMPORTED)
    find_package(AC QUIET)
    if(NOT AC_FOUND)
        message(STATUS "dep: ac not found, will be fetched online.")
        include(FetchContent)
        FetchContent_Declare(
            ac
            GIT_REPOSITORY https://github.com/TianZerL/Anime4KCPP.git
            GIT_TAG master
        )
        set(AC_BUILD_CLI OFF CACHE BOOL "Anime4KCPP option" FORCE)
        FetchContent_MakeAvailable(ac)
    endif()
    target_link_libraries(dep_ac INTERFACE AC::Core)
    add_library(dep::ac ALIAS dep_ac)
endif()

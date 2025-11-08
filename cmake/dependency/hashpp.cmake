if(NOT TARGET dep::hashpp)
    add_library(dep_hashpp INTERFACE IMPORTED)
    if(AC_PATH_HASHPP)
        set(dep_hashpp_PATH ${AC_PATH_HASHPP})
    endif()
    find_path(dep_hashpp_INCLUDE
        NAMES hashpp.h
        PATHS ${dep_hashpp_PATH}
    )
    if(dep_hashpp_INCLUDE)
        target_include_directories(dep_hashpp INTERFACE $<BUILD_INTERFACE:${dep_hashpp_INCLUDE}>)
    else()
        message(STATUS "dep: hashpp not found, will be fetched online.")
        include(FetchContent)
        FetchContent_Declare(
            hashpp
            GIT_REPOSITORY https://github.com/D7EAD/HashPlusPlus.git
            GIT_TAG main
        )
        FetchContent_MakeAvailable(hashpp)
        target_include_directories(dep_hashpp INTERFACE $<BUILD_INTERFACE:${hashpp_SOURCE_DIR}/include>)
    endif()
    add_library(dep::hashpp ALIAS dep_hashpp)
endif()

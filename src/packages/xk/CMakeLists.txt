if(${PACKAGE_XK})
    add_library(package_xk STATIC
            "xk.cc"
            "xk.h"
            )
    if(STATIC)
      target_compile_definitions(package_xk PUBLIC "XK_STATIC")
    endif()

    # PACKAGE_XK 使用自己的 PostgreSQL 实现，独立于 PACKAGE_DB
    find_package(PostgreSQL REQUIRED)
    message(STATUS "PACKAGE_XK USE_POSTGRES, Found version ${PostgreSQL_VERSION_STRING}")
    target_include_directories(package_xk SYSTEM PUBLIC ${PostgreSQL_INCLUDE_DIRS})
    target_link_libraries(package_xk PRIVATE -L${PostgreSQL_LIBRARY_DIRS} ${PostgreSQL_LIBRARIES})
endif()

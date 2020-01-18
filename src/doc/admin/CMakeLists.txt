

add_subdirectory(mof)
add_subdirectory(pic)

add_custom_target(doc
    COMMAND cp ${CMAKE_SOURCE_DIR}/doc/admin/*.rst ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND cp ${CMAKE_SOURCE_DIR}/doc/admin/pic/*.svg ${CMAKE_CURRENT_BINARY_DIR}/pic
    COMMAND sphinx-build
        -b html
        -c ${CMAKE_CURRENT_SOURCE_DIR}
        -D "version=${OPENLMI_VERSION}" -D "release=${OPENLMI_VERSION}"
        ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/html
)

add_dependencies(doc
    doc-mof doc-pic)

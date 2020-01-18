
find_program(PLANTUML plantuml)

# Generate
if (PLANTUML)
    # We have plantuml, svgs will be regenerated
    add_custom_target(doc-pic
        COMMAND ./make-svg.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
else (PLANTUML)
    # We don't have plantuml, so we depend on svgs in tarball
    add_custom_target(doc-pic)
endif (PLANTUML)

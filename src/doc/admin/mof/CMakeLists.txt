
add_custom_target(doc-mof
    COMMAND openlmi-doc-class2rst
            -H ${CMAKE_CURRENT_SOURCE_DIR}/header.txt
            --schema /usr/share/mof/cim-current/CIM_Schema.mof
            --schema /usr/share/openlmi-providers/05_LMI_Qualifiers.mof
            --schema /usr/share/openlmi-providers/30_LMI_Jobs.mof
            --mof ${CMAKE_SOURCE_DIR}/mof/60_LMI_Networking.mof
            CIM_ComputerSystem CIM_ConcreteJob
    DEPENDS ${CMAKE_SOURCE_DIR}/mof/*.mof
)

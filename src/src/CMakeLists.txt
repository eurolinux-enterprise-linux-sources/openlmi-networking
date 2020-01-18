
set(PROVIDER_NAME Networking)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Networking.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(basic_SRCS
    network.c
    ipconfig.c
    globals.c
    port.c
    setting.c
    connection.c
    activeconnection.c
    dbus_wrapper.c
    nm_support.c
    indications.c
    job.c
)

set(nm_SRCS
    network_nm.c
    port_nm.c
    setting_nm.c
    connection_nm.c
    activeconnection_nm.c
    ipassignmentsettingdata.c
    ipnetworkconnection.c
    network_job.c
)

konkretcmpi_generate(${MOF}
                     CIM_PROVIDERS
                     CIM_HEADERS
                     CIM_CLASSES
                     ${OPENLMI_QUALIFIERS_MOF} ${OPENLMI_JOBS_MOF}
)

add_library(${LIBRARY_NAME} SHARED
            ${basic_SRCS}
            ref_factory.c
            ${nm_SRCS}
            ${CIM_PROVIDERS}
            ${CIM_HEADERS}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMPI_INCLUDE_DIR}
    ${KONKRETCMPI_INCLUDE_DIR} ${DBUSGLIB_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS}
    ${NM_INCLUDE_DIR} ${UUID_INCLUDE_DIR} ${OPENLMI_INCLUDE_DIR}
)

target_link_libraries(${LIBRARY_NAME} ${KONKRETCMPI_LIBRARIES} ${DBUSGLIB_LIBRARIES}
    ${GLIB_LIBRARIES} ${UUID_LIBRARIES} pthread ${OPENLMI_LIBRARIES})

# Profile Registration
set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_Networking_Profile.mof")
configure_file("${CMAKE_SOURCE_DIR}/mof/90_LMI_Networking_Profile.mof" ${TARGET_MOF} COPYONLY)

macro(register SKELETON CLASSES OUT_FILE)
    file(READ ${SKELETON} INSTANCE_SKEL)
    foreach (CLASS ${CLASSES})
        string(REPLACE "\@CLASS\@" ${CLASS} INSTANCE "${INSTANCE_SKEL}")
        string(REPLACE "\@VERSION\@" ${OPENLMI_VERSION} INSTANCE "${INSTANCE}")
        file(APPEND ${OUT_FILE} "${INSTANCE}")
    endforeach (CLASS ${CLASSES})
endmacro(register SKELETON CLASSES OUT_FILE)

# DSP1116 - IP Configuration Profile Registration
set(DSP1116
    "LMI_BindsToLANEndpoint;LMI_DHCPSettingData;LMI_DNSProtocolEndpoint;"
    "LMI_IPElementSettingData;LMI_IPVersionElementSettingData;"
    "LMI_EndpointForIPNetworkConnection;"
    "LMI_ExtendedStaticIPAssignmentSettingData;LMI_NetworkHostedAccessPoint;"
    "LMI_HostedIPConfigurationService;LMI_IPAssignmentSettingData;"
    "LMI_IPConfigurationService;LMI_IPNetworkConnection;LMI_IPProtocolEndpoint;"
    "LMI_IPVersionSettingData;LMI_OrderedIPAssignmentComponent;"
    "LMI_NetworkRemoteAccessAvailableToElement;"
    "LMI_NetworkRemoteServiceAccessPoint;LMI_NetworkSAPSAPDependency;"
    "LMI_IPConfigurationServiceAffectsElement")

# DSP1035 - Host LAN Network Port Profile Registration
set(DSP1035
    "LMI_NetworkDeviceSAPImplementation;LMI_NetworkElementCapabilities;"
    "LMI_NetworkEnabledLogicalElementCapabilities;LMI_NetworkHostedAccessPoint;"
    "LMI_LANEndpoint;LMI_EthernetPort;LMI_NetworkSystemDevice")

# DSP1036 - IP Interface Profile Registration
set(DSP1036
    "LMI_BindsToLANEndpoint;LMI_NetworkElementCapabilities;"
    "LMI_NetworkEnabledLogicalElementCapabilities;LMI_IPElementSettingData;"
    "LMI_NetworkHostedAccessPoint;LMI_IPAssignmentSettingData;"
    "LMI_IPConfigurationService;LMI_IPProtocolEndpoint;"
    "LMI_OrderedIPAssignmentComponent;LMI_NetworkRemoteAccessAvailableToElement;"
    "LMI_NetworkRemoteServiceAccessPoint;"
    "LMI_IPConfigurationServiceAffectsElement;"
    "LMI_ExtendedStaticIPAssignmentSettingData")

register("DSP1116.mof.skel" "${DSP1116}" ${TARGET_MOF})
register("DSP1035.mof.skel" "${DSP1035}" ${TARGET_MOF})
register("DSP1036.mof.skel" "${DSP1036}" ${TARGET_MOF})
register("OpenLMI-Networking.mof.skel" "${CIM_CLASSES}" ${TARGET_MOF})

# Create registration file
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-networking "${TARGET_MOF}")

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi/)
install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-networking/)

add_subdirectory(unittest)

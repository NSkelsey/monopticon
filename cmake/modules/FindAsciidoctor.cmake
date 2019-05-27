#
# - Find Asciidoctor
# Sets:
#  ASCIIDOCTOR_EXECUTABLE
#

INCLUDE(FindChocolatey)

FIND_PROGRAM(ASCIIDOCTOR_EXECUTABLE
    NAMES
        asciidoctorj
        asciidoctor
    PATHS
        /bin
        /usr/bin
        /usr/local/bin
        ${CHOCOLATEY_BIN_PATH}
    DOC "Path to Asciidoctor or AsciidoctorJ"
)

if(ASCIIDOCTOR_EXECUTABLE)
    # The AsciidctorJ wrapper script sets -Xmx256m. This isn't enough
    # for the User's Guide.
    set(_asciidoctorj_opts -Xmx800m $ENV{ASCIIDOCTORJ_OPTS})
    execute_process( COMMAND ${ASCIIDOCTOR_EXECUTABLE} --version OUTPUT_VARIABLE _ad_full_version )
    separate_arguments(_ad_full_version)
    list(GET _ad_full_version 1 ASCIIDOCTOR_VERSION)

    function(set_asciidoctor_target_properties _target)
        set_target_properties(${_target} PROPERTIES
            FOLDER "Docbook"
            EXCLUDE_FROM_DEFAULT_BUILD True
            )
    endfunction(set_asciidoctor_target_properties)

    set (_asciidoctor_common_args
        --attribute build_dir=${CMAKE_CURRENT_BINARY_DIR}
        --require ${CMAKE_CURRENT_SOURCE_DIR}/asciidoctor-macros/commaize-block.rb
        --require ${CMAKE_CURRENT_SOURCE_DIR}/asciidoctor-macros/cveidlink-inline-macro.rb
        --require ${CMAKE_CURRENT_SOURCE_DIR}/asciidoctor-macros/wsbuglink-inline-macro.rb
        --require ${CMAKE_CURRENT_SOURCE_DIR}/asciidoctor-macros/wssalink-inline-macro.rb
    )

    if(CMAKE_VERSION VERSION_LESS 3.1)
        set(_env_command ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/env.cmake)
    else()
        set(_env_command ${CMAKE_COMMAND} -E env)
    endif()

    set(_asciidoctor_common_command ${_env_command}
        TZ=UTC ASCIIDOCTORJ_OPTS="${_asciidoctorj_opts}"
        ${ASCIIDOCTOR_EXECUTABLE}
        ${_asciidoctor_common_args}
    )

    MACRO( ASCIIDOCTOR2DOCBOOK _asciidocsource )
        GET_FILENAME_COMPONENT( _source_base_name ${_asciidocsource} NAME_WE )
        set( _output_xml ${_source_base_name}.xml )

        add_custom_command(
            OUTPUT
                ${_output_xml}
            COMMAND ${_asciidoctor_common_command}
                --backend docbook
                --out-file ${_output_xml}
                ${CMAKE_CURRENT_SOURCE_DIR}/${_asciidocsource}
            DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/${_asciidocsource}
                ${ARGN}
        )
        add_custom_target(generate_${_output_xml} DEPENDS ${_output_xml})
        set_asciidoctor_target_properties(generate_${_output_xml})
        unset(_output_xml)
    ENDMACRO()

    # Currently single page only.
    MACRO( ASCIIDOCTOR2HTML _asciidocsource )
        GET_FILENAME_COMPONENT( _source_base_name ${_asciidocsource} NAME_WE )
        set( _output_html ${_source_base_name}.html )

        ADD_CUSTOM_COMMAND(
            OUTPUT
                ${_output_html}
            COMMAND ${_asciidoctor_common_command}
                --backend html
                --out-file ${_output_html}
                ${CMAKE_CURRENT_SOURCE_DIR}/${_asciidocsource}
            DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/${_asciidocsource}
                ${ARGN}
        )
        add_custom_target(generate_${_output_html} DEPENDS ${_output_html})
        set_asciidoctor_target_properties(generate_${_output_html})
        unset(_output_html)
    ENDMACRO()

    MACRO( ASCIIDOCTOR2TXT _asciidocsource )
        GET_FILENAME_COMPONENT( _source_base_name ${_asciidocsource} NAME_WE )
        set( _output_html ${_source_base_name}.html )
        set( _output_txt ${_source_base_name}.txt )

        ADD_CUSTOM_COMMAND(
        OUTPUT
                ${_output_txt}
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/html2text.py
                ${_output_html}
                > ${_output_txt}
        DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/${_asciidocsource}
                ${_output_html}
                ${ARGN}
        )
        unset(_output_html)
        unset(_output_txt)
    ENDMACRO()

    # news: release-notes.txt
    #         ${CMAKE_COMMAND} -E copy_if_different release-notes.txt ../NEWS

    FIND_PROGRAM(ASCIIDOCTOR_PDF_EXECUTABLE
        NAMES
            asciidoctorj
            asciidoctor-pdf
        PATHS
            /bin
            /usr/bin
            /usr/local/bin
            ${CHOCOLATEY_BIN_PATH}
        DOC "Path to Asciidoctor PDF or AsciidoctorJ"
    )

    if(ASCIIDOCTOR_PDF_EXECUTABLE)

        set(_asciidoctor_pdf_common_command ${_env_command}
            TZ=UTC ASCIIDOCTORJ_OPTS="${_asciidoctorj_opts}"
            ${ASCIIDOCTOR_PDF_EXECUTABLE}
            --require asciidoctor-pdf
            --backend pdf
            ${_asciidoctor_common_args}
        )

        MACRO( ASCIIDOCTOR2PDF _asciidocsource )
            GET_FILENAME_COMPONENT( _source_base_name ${_asciidocsource} NAME_WE )
            set( _output_pdf ${_source_base_name}.pdf )

            ADD_CUSTOM_COMMAND(
                OUTPUT
                    ${_output_pdf}
                COMMAND ${_asciidoctor_common_command}
                    --backend pdf
                    ${_asciidoctor_common_args}
                    --out-file ${_output_pdf}
                    ${CMAKE_CURRENT_SOURCE_DIR}/${_asciidocsource}
                DEPENDS
                    ${CMAKE_CURRENT_SOURCE_DIR}/${_asciidocsource}
                    ${ARGN}
            )
            add_custom_target(generate_${_output_pdf} DEPENDS ${_output_pdf})
            set_asciidoctor_target_properties(generate_${_output_pdf})
            unset(_output_pdf)
        ENDMACRO()

    else(ASCIIDOCTOR_PDF_EXECUTABLE)

        MACRO( ASCIIDOCTOR2PDF _asciidocsource )
        ENDMACRO()

    endif(ASCIIDOCTOR_PDF_EXECUTABLE)

endif(ASCIIDOCTOR_EXECUTABLE)

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( ASCIIDOCTOR
    REQUIRED_VARS ASCIIDOCTOR_EXECUTABLE
    VERSION_VAR ASCIIDOCTOR_VERSION
    )

mark_as_advanced( ASCIIDOCTOR_EXECUTABLE )

include(FetchContent)


function(fetch_dependencies file_path)
    file(READ ${file_path} config_file)

    set(support_os "mac" "windows")

    list(FIND support_os ${_os_prebuilds} index)

    if (index EQUAL -1)
        message(FATAL_ERROR "not support ${_os_prebuilds} os fetch dependencies")
        return()
    endif()
    
    message(STATUS "setting up ${_os_prebuilds} ${_os_arch} prebuild dependencies")

    string(JSON prebuild_data GET ${config_file} prebuild ${_os_prebuilds} ${_os_arch})
    
    message(DEBUG "prebuild_data = ${prebuild_data}")

    string(JSON len LENGTH ${prebuild_data})

    if(NOT EXISTS ${dependencies_install_path})
        file(MAKE_DIRECTORY ${dependencies_install_path})
    endif()

    set(idx 0)
    while(idx LESS ${len})
        string(JSON prebuild GET ${prebuild_data} ${idx})
        message(DEBUG "prebuild:" ${prebuild})

        string(JSON repo_name GET ${prebuild} name)
        string(JSON version GET ${prebuild} version)
        string(JSON url GET ${prebuild} url) 
        
        message(DEBUG "name:" ${repo_name})
        message(DEBUG "version:" ${version})
        message(DEBUG "url:" ${url})
        
        string(REGEX MATCH "[^/]+$" filename ${url})
        message(DEBUG "filename = ${filename}")

        set(download_path "${dependencies_install_path}/${filename}")
        if(NOT EXISTS ${download_path})
            message(STATUS "Downloading ${url}")
            file(DOWNLOAD "${url}" "${download_path}"
                STATUS download_status)
    
            list(GET download_status 0 error_code)
            list(GET download_status 1 error_message)
            if(error_code GREATER 0)
                file(REMOVE "${download_path]}")
                message(STATUS "Downloading ${url} - Failure")
                message(FATAL_ERROR "Unable to download ${url}, failed with error: ${error_message}")
            else()
                message(STATUS "Downloading ${url} - done")
            endif()
        endif()

        set(destination "${dependencies_install_path}/${repo_name}")

        if(NOT EXISTS ${destination})
            file(MAKE_DIRECTORY ${destination})
            file(ARCHIVE_EXTRACT INPUT ${download_path} DESTINATION ${destination})
        endif()
        
        math(EXPR idx "${idx} + 1") 
    endwhile()


    string(JSON source_data GET ${config_file} source)
    string(JSON len LENGTH ${source_data})
    set(idx 0)

    while(idx LESS ${len})
        
        string(JSON source GET ${source_data} ${idx})
        message(DEBUG "source:" ${source})

        string(JSON url GET ${source} repo)
        string(JSON tag GET ${source} tag)
        string(JSON name GET ${source} name)

        message(DEBUG "name:" ${name})
        message(DEBUG "tag:" ${tag})
        message(DEBUG "url:" ${url})


        FetchContent_Declare(
            ${name}
            GIT_REPOSITORY    ${url}
            GIT_TAG           ${tag}
        )
    
        FetchContent_MakeAvailable(${name})
        math(EXPR idx "${idx} + 1") 
    endwhile()



endfunction(fetch_dependencies)

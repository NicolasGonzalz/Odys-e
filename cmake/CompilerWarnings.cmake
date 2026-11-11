# Options de compilation communes à toutes les cibles du projet.

# Activé en CI : un avertissement dans notre code y fait échouer la compilation.
option(ODYSSEE_WARNINGS_AS_ERRORS "Traiter les avertissements comme des erreurs" OFF)

function(odyssee_set_options target)
    if(MSVC)
        # /utf-8 : nos sources contiennent des accents (sans ça, MSVC les lit dans l'encodage local).
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(ODYSSEE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
            -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wnull-dereference)
        if(ODYSSEE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
        if(ODYSSEE_SANITIZERS)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

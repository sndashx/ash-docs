# Sets aggressive warning flags on a target. Cross-platform: GCC/Clang
# use -W flags, MSVC uses the /w14xxx equivalents listed in the phase 00
# spec (05-phase-00-skeleton.txt Step 0001.00.01).
function(ash_set_project_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /permissive- /w14242 /w14254 /w14263 /w14265 /w14266
            /w14267 /w14268 /w14269 /w14270 /w14271 /w14272 /w14273
            /w14274 /w14275 /w14276 /w14277 /w14278 /w14279 /w14280
            /w14281 /w14282 /w14283 /w14284 /w14285 /w14288 /w14289
            /w14290 /w14291 /w14292 /w14293 /w14294 /w14295 /w14296
            /w14297 /w14298 /w14299 /w14300 /w14301 /w14302 /w14303
            /w14304 /w14305 /w14306 /w14307 /w14308 /w14309 /w14310
            /w14311 /w14312 /w14313 /w14314 /w14315 /w14316 /w14317
            /w14318 /w14319 /w14320)
        if(ASH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion
            -Wsign-conversion -Wnull-dereference -Wdouble-promotion
            -Wformat=2)
        if(ASH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

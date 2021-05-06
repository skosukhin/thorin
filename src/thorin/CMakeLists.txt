set(THORIN_SOURCES
    analyses/cfg.cpp
    analyses/cfg.h
    analyses/domfrontier.cpp
    analyses/domfrontier.h
    analyses/domtree.cpp
    analyses/domtree.h
    analyses/free_defs.cpp
    analyses/free_defs.h
    analyses/looptree.cpp
    analyses/looptree.h
    analyses/schedule.cpp
    analyses/schedule.h
    analyses/scope.cpp
    analyses/scope.h
    analyses/verify.cpp
    analyses/verify.h
    be/c/c.cpp
    be/c/c.h
    be/codegen.cpp
    be/codegen.h
    be/emitter.h
    be/kernel_config.h
    continuation.cpp
    continuation.h
    debug.cpp
    debug.h
    def.cpp
    def.h
    enums.cpp
    enums.h
    primop.cpp
    primop.h
    rec_stream.cpp
    tables/allnodes.h
    tables/arithoptable.h
    tables/cmptable.h
    tables/mathoptable.h
    tables/nodetable.h
    tables/primtypetable.h
    transform/cleanup_world.cpp
    transform/cleanup_world.h
    transform/clone_bodies.cpp
    transform/clone_bodies.h
    transform/closure_conversion.cpp
    transform/closure_conversion.h
    transform/codegen_prepare.cpp
    transform/codegen_prepare.h
    transform/dead_load_opt.cpp
    transform/dead_load_opt.h
    transform/flatten_tuples.cpp
    transform/flatten_tuples.h
    transform/hoist_enters.cpp
    transform/hoist_enters.h
    transform/importer.cpp
    transform/importer.h
    transform/inliner.cpp
    transform/inliner.h
    transform/lift_builtins.cpp
    transform/lift_builtins.h
    transform/mangle.cpp
    transform/mangle.h
    transform/partial_evaluation.cpp
    transform/partial_evaluation.h
    transform/resolve_loads.cpp
    transform/resolve_loads.h
    transform/split_slots.cpp
    transform/split_slots.h
    type.cpp
    type.h
    util/array.h
    util/cast.h
    util/hash.cpp
    util/hash.h
    util/indexmap.h
    util/indexset.h
    util/iterator.h
    util/stream.cpp
    util/stream.h
    util/symbol.cpp
    util/symbol.h
    util/types.h
    util/utility.h
    world.cpp
    world.h
)

if(LLVM_FOUND)
    list(APPEND THORIN_SOURCES
        be/llvm/amdgpu.cpp
        be/llvm/amdgpu.h
        be/llvm/cpu.cpp
        be/llvm/cpu.h
        be/llvm/llvm.cpp
        be/llvm/llvm.h
        be/llvm/nvvm.cpp
        be/llvm/nvvm.h
        be/llvm/parallel.cpp
        be/llvm/runtime.cpp
        be/llvm/runtime.h
        be/llvm/runtime.inc
        be/llvm/vectorize.cpp
    )
endif()

# We currently assume that all '*.h' files in THORIN_SOURCES are public headers:
set(THORIN_PUBLIC_HEADERS ${THORIN_SOURCES})
list(FILTER THORIN_PUBLIC_HEADERS INCLUDE REGEX "\\.h$")

add_library(thorin ${THORIN_SOURCES})

target_include_directories(thorin PUBLIC
    $<BUILD_INTERFACE:${THORIN_SRC_BINARY_DIR}>
    $<BUILD_INTERFACE:${THORIN_SRC_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    ${Half_INCLUDE_DIRS}
)

if(LLVM_FOUND)
    set(Thorin_LLVM_COMPONENTS core support ipo target ${LLVM_TARGETS_TO_BUILD})

    if(RV_FOUND)
        target_include_directories(thorin SYSTEM PRIVATE ${RV_INCLUDE_DIRS})
        target_link_libraries(thorin PRIVATE ${RV_LIBRARIES})

        list(APPEND Thorin_LLVM_COMPONENTS analysis passes transformutils)
    endif()

    # LLVM headers are exposed via the public headers of Impala:
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
    target_compile_definitions(thorin PUBLIC ${LLVM_DEFINITIONS_LIST})
    target_include_directories(thorin SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})

    # We do not use llvm_config here because it declares a private (unless
    # building a static library) dependency on LLVM libraries. However, the
    # public headers of Thorin trigger a direct dependency on LLVM. Therefore,
    # we do what llvm_config normally does but the dependencies on LLVM (and its
    # static components) are declared as public.
    if("${AnyDSL_LLVM_LINK_SHARED}" STREQUAL "USE_SHARED" AND DEFINED LLVM_DYLIB_COMPONENTS)
        if("${LLVM_DYLIB_COMPONENTS}" STREQUAL "all")
            set(Thorin_LLVM_COMPONENTS "")
        else()
            list(REMOVE_ITEM Thorin_LLVM_COMPONENTS ${LLVM_DYLIB_COMPONENTS})
        endif()
        target_link_libraries(thorin PUBLIC LLVM)
    endif()
    llvm_map_components_to_libnames(Thorin_LLVM_COMPONENT_LIBRARIES ${Thorin_LLVM_COMPONENTS})
    target_link_libraries(thorin PUBLIC ${Thorin_LLVM_COMPONENT_LIBRARIES})
endif()

if(COLORIZE_OUTPUT)
    target_compile_definitions(thorin PRIVATE COLORIZE_LOG)
endif()

# Configure config.h definitions:
if(LLVM_FOUND)
    set(THORIN_ENABLE_LLVM TRUE)

    if(RV_FOUND)
        set(THORIN_ENABLE_RV TRUE)
    endif()
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(THORIN_ENABLE_CHECKS TRUE)
endif()
if(THORIN_PROFILE)
    set(THORIN_ENABLE_PROFILING TRUE)
endif()

# Generate and install config.h:
configure_file(config.h.in config.h @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thorin
)

# Install public headers preserving the directory layout:
foreach(header ${THORIN_PUBLIC_HEADERS})
    get_filename_component(dir ${header} DIRECTORY)
    install(FILES ${header} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thorin/${dir})
endforeach()

# Install the library:
install(TARGETS thorin
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        EXPORT thorin-targets
)

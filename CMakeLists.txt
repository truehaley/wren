cmake_minimum_required(VERSION 3.24)

project(wren
        VERSION 0.4
        DESCRIPTION "Wren"
        LANGUAGES C CXX ASM
)

# always generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


###################
# Wren static library
###################
add_library(wrenlib STATIC)

target_compile_definitions(wrenlib PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

target_compile_definitions(wrenlib PUBLIC
    ${WREN_NUMBER}
)

target_include_directories(wrenlib PUBLIC
    src/include
)

target_include_directories(wrenlib PRIVATE
    src/vm
    src/optional
)

target_sources(wrenlib PRIVATE
    src/vm/wren_compiler.c
    src/vm/wren_core.c
    src/vm/wren_debug.c
    src/vm/wren_lexer.c
    src/vm/wren_primitive.c
    src/vm/wren_utils.c
    src/vm/wren_value.c
    src/vm/wren_vm.c
    src/optional/wren_opt_meta.c
    src/optional/wren_opt_random.c
)

function(generate_wren_include name source output)
    add_custom_target(generate-${name} DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${output})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${output}
        COMMENT "Generating ${output}"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/util/wren_to_c_string.py ${CMAKE_CURRENT_SOURCE_DIR}/${output} ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        VERBATIM
    )
    add_dependencies(wrenlib generate-${name})
endfunction(generate_wren_include)

generate_wren_include(wren_core
    src/vm/wren_core.wren src/vm/generated/wren_core.wren.inc)
generate_wren_include(wren_opt_random
    src/optional/wren_opt_random.wren src/optional/generated/wren_opt_random.wren.inc)
generate_wren_include(wren_opt_meta
    src/optional/wren_opt_meta.wren src/optional/generated/wren_opt_meta.wren.inc)


###################
# Wren test app
###################
add_executable(wren_test)
target_sources(wren_test PRIVATE
    test/main.c
    test/test.c
    test/api/api_tests.c
    test/api/benchmark.c
    test/api/call_calls_foreign.c
    test/api/call_wren_call_root.c
    test/api/call.c
    test/api/error.c
    test/api/foreign_class.c
    test/api/get_variable.c
    test/api/handle.c
    test/api/lists.c
    test/api/maps.c
    test/api/new_vm.c
    test/api/reset_stack_after_call_abort.c
    test/api/reset_stack_after_foreign_construct.c
    test/api/resolution.c
    test/api/slots.c
    test/api/user_data.c
)
target_link_libraries(wren_test
    wrenlib
)

###################
# Wren tests
###################
set(CMAKE_SKIP_TEST_ALL_DEPENDENCY FALSE)
include(CTest)
add_custom_target(tests)
add_dependencies(tests wren_test)
function(test_suite suite)
    if(WREN_NUMBER STREQUAL WREN_FLOAT32)
        add_test(NAME test-${suite}
            COMMAND ${CMAKE_SOURCE_DIR}/util/test.py --float32 --executable ${CMAKE_BINARY_DIR}/wren_test ${suite}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    else()
        add_test(NAME test-${suite}
            COMMAND ${CMAKE_SOURCE_DIR}/util/test.py --executable ${CMAKE_BINARY_DIR}/wren_test ${suite}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()
endfunction(test_suite)

test_suite(api)

test_suite(core/bool)
test_suite(core/class)
test_suite(core/fiber)
test_suite(core/function)
test_suite(core/list)
test_suite(core/map)
test_suite(core/map_entry)
test_suite(core/null)
test_suite(core/number)
test_suite(core/object)
test_suite(core/range)
test_suite(core/sequence)
test_suite(core/string)
test_suite(core/string_byte_sequence)
test_suite(core/string_code_point_sequence)
test_suite(core/system)

test_suite(language/assignment)
test_suite(language/break)
test_suite(language/class)
test_suite(language/closure)
test_suite(language/comments)
test_suite(language/conditional)
test_suite(language/constructor)
test_suite(language/continue)
test_suite(language/fiber)
test_suite(language/field)
test_suite(language/for)
test_suite(language/foreign)
test_suite(language/function)
test_suite(language/if)
test_suite(language/implicit_receiver)
test_suite(language/inheritance)
test_suite(language/interpolation)
test_suite(language/list)
test_suite(language/logical_operator)
test_suite(language/map)
test_suite(language/method)
test_suite(language/misc)
test_suite(language/module)
test_suite(language/nonlocal)
test_suite(language/null)
test_suite(language/number)
test_suite(language/return)
test_suite(language/setter)
test_suite(language/shebang)
test_suite(language/static_field)
test_suite(language/string)
test_suite(language/super)
test_suite(language/this)
test_suite(language/variable)
test_suite(language/while)

test_suite(limit)
test_suite(meta)
test_suite(random)
test_suite(regression)

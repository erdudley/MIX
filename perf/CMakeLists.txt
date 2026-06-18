add_executable(mix-perf perf_main.c)
target_compile_definitions(mix-perf PRIVATE _POSIX_C_SOURCE=199309L)
target_link_libraries(mix-perf PRIVATE mix::mix)
mix_set_warnings(mix-perf)
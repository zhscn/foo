add_executable(foo main.cc)
target_link_libraries(foo PRIVATE full)

function(foo_add_test name)
  add_executable(${name} ${name}.cc)
  target_link_libraries(${name} PRIVATE full GTest::gtest_main)
  add_test(NAME ${name} COMMAND ${name})
endfunction(foo_add_test)

foo_add_test(outcome_test)
foo_add_test(serde_test)

add_executable(mcs termio-select.cc)
target_link_libraries(mcs PRIVATE full)

pybind11_add_module(foopy py.cc)
install(TARGETS foopy DESTINATION .)

add_library(tty_device SHARED tty_device.c)

add_executable(t2d t2d.cc)
target_link_libraries(t2d PRIVATE full)

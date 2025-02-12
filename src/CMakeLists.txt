add_executable(foo main.cc)
target_link_libraries(foo PRIVATE full)

function(foo_add_test name)
  add_executable(${name} ${name}.cc)
  target_link_libraries(${name} PRIVATE full GTest::gtest_main)
  add_test(NAME ${name} COMMAND ${name})
endfunction(foo_add_test)

foo_add_test(outcome_test)

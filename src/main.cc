#include "version.hh"

#include <fmt/format.h>

int main() {
  fmt::println("{}\n{}\n{}", get_project_name(), get_project_version(),
               get_project_description());
}

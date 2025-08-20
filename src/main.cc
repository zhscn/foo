#include "version.hh"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/flags/usage_config.h>
#include <absl/strings/match.h>
#include <fmt/format.h>

void handle_args(int argc, char** argv) {
  absl::FlagsUsageConfig cfg;
  cfg.version_string = []() -> std::string { return "0.1.0\n"; };
  cfg.contains_help_flags = [](absl::string_view sv) -> bool {
    return !absl::StrContains(sv, "absl");
  };
  cfg.normalize_filename = [](absl::string_view sv) -> std::string {
    size_t len = 0;
    if (auto n = sv.rfind("absl/"); n != absl::string_view::npos) {
      len = n;
    } else if (auto n = sv.rfind("root/"); n != absl::string_view::npos) {
      len = n;
    }
    return {sv.data() + len, sv.length() - len};
  };

  absl::SetFlagsUsageConfig(cfg);
  absl::SetProgramUsageMessage("usage");
  absl::ParseCommandLine(argc, argv);
}

int main() {
  fmt::println("{}\n{}\n{}", get_project_name(), get_project_version(),
               get_project_description());
}

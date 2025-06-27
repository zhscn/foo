#pragma once
#include <absl/log/log.h>
#include <fmt/format.h>

/// ERROR: --stderrthreshold=2 (default level)
/// WARNING: --stderrthreshold=1
/// INFO: --stderrthreshold=0
/// DEBUG: --stderrthreshold=0 --v=1
/// TRACE: --stderrthreshold=0 --v=2

#define IMPL_LOGF_(_log, _serverity, _spec, ...) \
  _log(_serverity) << fmt::format("[{}] " _spec, \
                                  __func__ __VA_OPT__(, ) __VA_ARGS__)

#define FATALF(...) IMPL_LOGF_(LOG, FATAL, __VA_ARGS__)
#define ERRORF(...) IMPL_LOGF_(LOG, ERROR, __VA_ARGS__)
#define WARNF(...) IMPL_LOGF_(LOG, WARNING, __VA_ARGS__)
#define INFOF(...) IMPL_LOGF_(VLOG, 0, __VA_ARGS__)
#define DEBUGF(...) IMPL_LOGF_(VLOG, 1, __VA_ARGS__)
#define TRACEF(...) IMPL_LOGF_(VLOG, 2, __VA_ARGS__)

#define FATAL(...) LOG(FATAL) << fmt::format(__VA_ARGS__)
#define ERROR(...) LOG(ERROR) << fmt::format(__VA_ARGS__)
#define WARN(...) LOG(WARNING) << fmt::format(__VA_ARGS__)
#define INFO(...) LOG(INFO) << fmt::format(__VA_ARGS__)
#define DEBUG(...) LOG(DEBUG) << fmt::format(__VA_ARGS__)
#define TRACE(...) LOG(TRACE) << fmt::format(__VA_ARGS__)

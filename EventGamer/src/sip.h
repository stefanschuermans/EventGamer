#pragma once

#include "config.h"

namespace eventgamer {

// Runs on main until a shutdown request, refreshing registration automatically.
void run_sip(const SipConfig &config, const AudioConfig &audio, const OscConfig &osc,
             bool sip_trace = false);

} // namespace eventgamer

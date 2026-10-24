#pragma once

#include "config.h"

namespace eventgamer {

// Runs on main until a shutdown request, refreshing registration automatically.
void run_sip(const SipConfig &config);

} // namespace eventgamer

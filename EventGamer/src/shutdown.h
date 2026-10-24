#pragma once

namespace eventgamer {

// Install before starting SIP. Handlers only request shutdown; cleanup runs on main.
void install_shutdown_handlers();
bool shutdown_requested();

} // namespace eventgamer

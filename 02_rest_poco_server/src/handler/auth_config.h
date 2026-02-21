#pragma once

#include <string>

namespace handlers {

// JWT signing secret from JWT_SECRET env var. Used for token creation and verification.
extern std::string g_jwtSecret;

} // namespace handlers

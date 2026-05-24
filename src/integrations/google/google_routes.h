#pragma once

#include <crow.h>

#include <memory>

#include "core/http/middleware.h"
#include "integrations/google/calendar_service.h"
#include "integrations/google/google_oauth.h"

void registerGoogleRoutes(crow::App<RequestTimerMiddleware>& app,
                          const std::shared_ptr<GoogleOAuth>& googleOAuth,
                          const std::shared_ptr<CalendarService>& calendarService);

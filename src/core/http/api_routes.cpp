#include "core/http/api_routes.h"

#include "core/http/static_routes.h"
#include "features/meals/meals_routes.h"
#include "features/workouts/workouts_routes.h"
#include "integrations/google/google_routes.h"

void setupRoutes(crow::App<RequestTimerMiddleware>& app, std::shared_ptr<DBManager> dbManager,
                 MealFactory& factory, const Config& /*config*/,
                 const std::shared_ptr<GoogleOAuth>& googleOAuth,
                 const std::shared_ptr<CalendarService>& calendarService) {
    registerMealsRoutes(app, dbManager, factory);
    registerWorkoutsRoutes(app, dbManager);
    registerGoogleRoutes(app, googleOAuth, calendarService);
    registerStaticRoutes(app);
}

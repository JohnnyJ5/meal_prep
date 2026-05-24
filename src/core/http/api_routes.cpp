#include "core/http/api_routes.h"

#include "core/http/static_routes.h"
#include "features/meals/meals_routes.h"
#include "features/workouts/workouts_routes.h"
#include "integrations/google/google_routes.h"

void setupRoutes(crow::App<RequestTimerMiddleware>& app,
                 std::shared_ptr<MealsRepository> meals,
                 std::shared_ptr<WorkoutsRepository> workouts, MealFactory& factory,
                 const std::shared_ptr<GoogleOAuth>& googleOAuth,
                 const std::shared_ptr<CalendarService>& calendarService) {
    registerMealsRoutes(app, std::move(meals), factory);
    registerWorkoutsRoutes(app, std::move(workouts));
    registerGoogleRoutes(app, googleOAuth, calendarService);
    registerStaticRoutes(app);
}

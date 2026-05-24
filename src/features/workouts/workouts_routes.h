#pragma once

#include <crow.h>

#include <memory>

#include "core/http/middleware.h"
#include "features/workouts/workouts_repository.h"

void registerWorkoutsRoutes(crow::App<RequestTimerMiddleware>& app,
                            std::shared_ptr<WorkoutsRepository> workouts);

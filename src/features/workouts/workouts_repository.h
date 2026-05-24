#pragma once

#include <memory>
#include <vector>

#include "core/db/db_connection.h"
#include "features/workouts/workout.h"

/// SQLite-backed CRUD for workouts (and their blocks/exercises) and the
/// reusable workout templates.
class WorkoutsRepository {
   public:
    explicit WorkoutsRepository(std::shared_ptr<DbConnection> conn);

    bool addWorkout(Workout& workout);
    bool updateWorkout(const Workout& workout);
    bool deleteWorkout(int id);
    Workout getWorkout(int id);
    std::vector<WorkoutSummary> listWorkouts();

    bool addTemplate(WorkoutTemplate& tmpl);
    bool updateTemplate(const WorkoutTemplate& tmpl);
    bool deleteTemplate(int id);
    WorkoutTemplate getTemplate(int id);
    std::vector<WorkoutTemplateSummary> listTemplates();

   private:
    std::shared_ptr<DbConnection> d_conn;
};

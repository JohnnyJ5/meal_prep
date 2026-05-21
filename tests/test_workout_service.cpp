#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>

#include "../src/db_manager.h"
#include "../src/workout.h"

class WorkoutTest : public ::testing::Test {
   protected:
    void SetUp() override {
        unsetenv("MEAL_PREP_TOKEN_KEY");
        db = std::make_unique<DBManager>(":memory:");
        ASSERT_TRUE(db->initializeSchema());
    }
    void TearDown() override { unsetenv("MEAL_PREP_TOKEN_KEY"); }

    std::unique_ptr<DBManager> db;

    Workout buildSampleCircuit() {
        Workout w;
        w.name = "Saturday circuit";
        w.performed_on = "2026-05-21";
        w.duration_seconds = 25 * 60;
        w.notes = "felt strong";
        w.created_at = 1716300000;

        WorkoutBlock circuit;
        circuit.type = BlockType::CIRCUIT;
        circuit.rounds = 3;
        circuit.rest_seconds = 60;

        WorkoutExercise run;
        run.name = "Run";
        run.type = ExerciseType::DISTANCE;
        run.distance = 0.25;
        run.distance_unit = "mi";
        circuit.exercises.push_back(run);

        WorkoutExercise dl;
        dl.name = "Deadlift";
        dl.type = ExerciseType::REPS;
        dl.sets = 1;
        dl.reps = 20;
        dl.weight_lbs = 135.0;
        circuit.exercises.push_back(dl);

        WorkoutExercise pushup;
        pushup.name = "Pushup";
        pushup.type = ExerciseType::REPS;
        pushup.sets = 1;
        pushup.reps = 20;
        circuit.exercises.push_back(pushup);

        w.blocks.push_back(circuit);
        return w;
    }
};

TEST_F(WorkoutTest, AddAndGetRoundTrip) {
    Workout w = buildSampleCircuit();
    ASSERT_TRUE(db->addWorkout(w));
    EXPECT_GT(w.id, 0);

    Workout loaded = db->getWorkout(w.id);
    EXPECT_EQ(loaded.id, w.id);
    EXPECT_EQ(loaded.name, "Saturday circuit");
    EXPECT_EQ(loaded.performed_on, "2026-05-21");
    EXPECT_EQ(loaded.duration_seconds, 25 * 60);
    EXPECT_EQ(loaded.notes, "felt strong");
    ASSERT_EQ(loaded.blocks.size(), 1u);

    const auto &block = loaded.blocks[0];
    EXPECT_EQ(block.type, BlockType::CIRCUIT);
    EXPECT_EQ(block.rounds, 3);
    EXPECT_EQ(block.rest_seconds, 60);
    ASSERT_EQ(block.exercises.size(), 3u);

    EXPECT_EQ(block.exercises[0].name, "Run");
    EXPECT_EQ(block.exercises[0].type, ExerciseType::DISTANCE);
    EXPECT_DOUBLE_EQ(block.exercises[0].distance, 0.25);
    EXPECT_EQ(block.exercises[0].distance_unit, "mi");

    EXPECT_EQ(block.exercises[1].name, "Deadlift");
    EXPECT_EQ(block.exercises[1].type, ExerciseType::REPS);
    EXPECT_EQ(block.exercises[1].reps, 20);
    EXPECT_DOUBLE_EQ(block.exercises[1].weight_lbs, 135.0);

    EXPECT_EQ(block.exercises[2].name, "Pushup");
    EXPECT_EQ(block.exercises[2].reps, 20);
}

TEST_F(WorkoutTest, ListReturnsSummariesNewestFirst) {
    Workout earlier;
    earlier.performed_on = "2026-05-19";
    earlier.duration_seconds = 600;
    earlier.created_at = 1;
    ASSERT_TRUE(db->addWorkout(earlier));

    Workout later;
    later.name = "Today";
    later.performed_on = "2026-05-21";
    later.duration_seconds = 900;
    later.created_at = 2;
    WorkoutBlock b;
    b.type = BlockType::STRAIGHT;
    WorkoutExercise e;
    e.name = "Squat";
    e.type = ExerciseType::REPS;
    e.sets = 5;
    e.reps = 5;
    e.weight_lbs = 225.0;
    b.exercises.push_back(e);
    later.blocks.push_back(b);
    ASSERT_TRUE(db->addWorkout(later));

    auto list = db->listWorkouts();
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0].name, "Today");
    EXPECT_EQ(list[0].exercise_count, 1);
    EXPECT_EQ(list[1].exercise_count, 0);
}

TEST_F(WorkoutTest, DeleteCascadesBlocksAndExercises) {
    Workout w = buildSampleCircuit();
    ASSERT_TRUE(db->addWorkout(w));
    int id = w.id;

    ASSERT_TRUE(db->deleteWorkout(id));
    Workout missing = db->getWorkout(id);
    EXPECT_EQ(missing.id, 0);
    EXPECT_TRUE(missing.blocks.empty());
}

TEST_F(WorkoutTest, UpdateReplacesBlocksAndExercises) {
    Workout w = buildSampleCircuit();
    ASSERT_TRUE(db->addWorkout(w));

    Workout updated;
    updated.id = w.id;
    updated.name = "Renamed";
    updated.performed_on = "2026-05-22";
    updated.duration_seconds = 1200;
    WorkoutBlock straight;
    straight.type = BlockType::STRAIGHT;
    WorkoutExercise plank;
    plank.name = "Plank";
    plank.type = ExerciseType::TIME;
    plank.duration_seconds = 60;
    straight.exercises.push_back(plank);
    updated.blocks.push_back(straight);

    ASSERT_TRUE(db->updateWorkout(updated));

    Workout loaded = db->getWorkout(w.id);
    EXPECT_EQ(loaded.name, "Renamed");
    EXPECT_EQ(loaded.performed_on, "2026-05-22");
    ASSERT_EQ(loaded.blocks.size(), 1u);
    ASSERT_EQ(loaded.blocks[0].exercises.size(), 1u);
    EXPECT_EQ(loaded.blocks[0].exercises[0].name, "Plank");
    EXPECT_EQ(loaded.blocks[0].exercises[0].type, ExerciseType::TIME);
    EXPECT_EQ(loaded.blocks[0].exercises[0].duration_seconds, 60);
}

TEST_F(WorkoutTest, BlockAndExerciseOrderingPreserved) {
    Workout w;
    w.performed_on = "2026-05-21";
    w.duration_seconds = 100;

    WorkoutBlock first;
    first.type = BlockType::STRAIGHT;
    WorkoutExercise a;
    a.name = "A";
    a.type = ExerciseType::REPS;
    a.sets = 1;
    a.reps = 1;
    WorkoutExercise b;
    b.name = "B";
    b.type = ExerciseType::REPS;
    b.sets = 1;
    b.reps = 1;
    WorkoutExercise c;
    c.name = "C";
    c.type = ExerciseType::REPS;
    c.sets = 1;
    c.reps = 1;
    first.exercises = {a, b, c};

    WorkoutBlock second;
    second.type = BlockType::CIRCUIT;
    second.rounds = 2;
    WorkoutExercise d;
    d.name = "D";
    d.type = ExerciseType::REPS;
    d.sets = 1;
    d.reps = 1;
    second.exercises = {d};

    w.blocks = {first, second};
    ASSERT_TRUE(db->addWorkout(w));

    Workout loaded = db->getWorkout(w.id);
    ASSERT_EQ(loaded.blocks.size(), 2u);
    EXPECT_EQ(loaded.blocks[0].type, BlockType::STRAIGHT);
    EXPECT_EQ(loaded.blocks[1].type, BlockType::CIRCUIT);
    ASSERT_EQ(loaded.blocks[0].exercises.size(), 3u);
    EXPECT_EQ(loaded.blocks[0].exercises[0].name, "A");
    EXPECT_EQ(loaded.blocks[0].exercises[1].name, "B");
    EXPECT_EQ(loaded.blocks[0].exercises[2].name, "C");
    EXPECT_EQ(loaded.blocks[1].exercises[0].name, "D");
}

TEST_F(WorkoutTest, EmptyNotesAndNameRoundTripAsEmpty) {
    Workout w;
    w.performed_on = "2026-05-21";
    w.duration_seconds = 0;
    // Single time-only exercise, no sets/reps
    WorkoutBlock block;
    block.type = BlockType::STRAIGHT;
    WorkoutExercise plank;
    plank.name = "Plank";
    plank.type = ExerciseType::TIME;
    plank.duration_seconds = 120;
    block.exercises.push_back(plank);
    w.blocks.push_back(block);

    ASSERT_TRUE(db->addWorkout(w));
    Workout loaded = db->getWorkout(w.id);
    EXPECT_EQ(loaded.name, "");
    EXPECT_EQ(loaded.notes, "");
    ASSERT_EQ(loaded.blocks.size(), 1u);
    ASSERT_EQ(loaded.blocks[0].exercises.size(), 1u);
    EXPECT_EQ(loaded.blocks[0].exercises[0].duration_seconds, 120);
    EXPECT_EQ(loaded.blocks[0].exercises[0].sets, 0);
    EXPECT_EQ(loaded.blocks[0].exercises[0].reps, 0);
}

TEST_F(WorkoutTest, GetMissingWorkoutReturnsEmpty) {
    Workout missing = db->getWorkout(99999);
    EXPECT_EQ(missing.id, 0);
    EXPECT_TRUE(missing.blocks.empty());
}

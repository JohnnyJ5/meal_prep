#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class ExerciseType {
    REPS,
    DISTANCE,
    TIME,
};

enum class BlockType {
    STRAIGHT,
    CIRCUIT,
};

struct WorkoutExercise {
    int id{0};
    int position{0};
    std::string name;
    ExerciseType type{ExerciseType::REPS};

    // REPS: sets * reps at weight_lbs
    int sets{0};
    int reps{0};
    double weight_lbs{0.0};

    // DISTANCE: distance + distance_unit (and optional duration_seconds)
    double distance{0.0};
    std::string distance_unit;  // "mi", "km", "m"

    // TIME: primary field is duration_seconds. Also used as optional for DISTANCE.
    int duration_seconds{0};

    // Optional rest between sets within this exercise (REPS) or between
    // distance/time intervals.
    int rest_seconds{0};
};

struct WorkoutBlock {
    int id{0};
    int position{0};
    BlockType type{BlockType::STRAIGHT};
    int rounds{1};
    int rest_seconds{0};  // rest between rounds (circuit) or between sets (straight)
    std::vector<WorkoutExercise> exercises;
};

struct Workout {
    int id{0};
    std::string name;
    std::string performed_on;  // YYYY-MM-DD
    int duration_seconds{0};
    std::string notes;
    int64_t created_at{0};  // unix timestamp
    std::vector<WorkoutBlock> blocks;
};

struct WorkoutSummary {
    int id{0};
    std::string name;
    std::string performed_on;
    int duration_seconds{0};
    int exercise_count{0};
};

inline std::string exerciseTypeToString(ExerciseType t) {
    switch (t) {
        case ExerciseType::REPS:
            return "reps";
        case ExerciseType::DISTANCE:
            return "distance";
        case ExerciseType::TIME:
            return "time";
    }
    return "reps";
}

inline ExerciseType exerciseTypeFromString(const std::string &s) {
    if (s == "distance") return ExerciseType::DISTANCE;
    if (s == "time") return ExerciseType::TIME;
    return ExerciseType::REPS;
}

inline std::string blockTypeToString(BlockType t) {
    switch (t) {
        case BlockType::CIRCUIT:
            return "circuit";
        case BlockType::STRAIGHT:
            return "straight";
    }
    return "straight";
}

inline BlockType blockTypeFromString(const std::string &s) {
    if (s == "circuit") return BlockType::CIRCUIT;
    return BlockType::STRAIGHT;
}

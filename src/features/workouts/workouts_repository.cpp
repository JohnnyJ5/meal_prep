#include "features/workouts/workouts_repository.h"

#include <sqlite3.h>

#include <string>
#include <utility>

WorkoutsRepository::WorkoutsRepository(std::shared_ptr<DbConnection> conn)
    : d_conn(std::move(conn)) {}

namespace {

void bindNullableInt(sqlite3_stmt* stmt, int idx, int value, bool hasValue) {
    if (hasValue) {
        sqlite3_bind_int(stmt, idx, value);
    } else {
        sqlite3_bind_null(stmt, idx);
    }
}

void bindNullableDouble(sqlite3_stmt* stmt, int idx, double value, bool hasValue) {
    if (hasValue) {
        sqlite3_bind_double(stmt, idx, value);
    } else {
        sqlite3_bind_null(stmt, idx);
    }
}

void bindNullableText(sqlite3_stmt* stmt, int idx, const std::string& value) {
    if (value.empty()) {
        sqlite3_bind_null(stmt, idx);
    } else {
        sqlite3_bind_text(stmt, idx, value.c_str(), -1, SQLITE_TRANSIENT);
    }
}

// Inserts blocks and exercises into the given pair of tables. Caller owns the
// transaction. The blocks table's FK column to the parent is named by
// parentFkCol (e.g. "workout_id" or "template_id"). The exercises table's FK
// to its block is always "block_id".
bool insertBlocksAndExercisesGeneric(sqlite3* db, int parentId,
                                     const std::vector<WorkoutBlock>& blocks,
                                     const std::string& blocksTable,
                                     const std::string& parentFkCol,
                                     const std::string& exercisesTable) {
    const std::string insertBlock = "INSERT INTO " + blocksTable + " (" + parentFkCol +
                                    ", position, block_type, rounds, rest_seconds) "
                                    "VALUES (?, ?, ?, ?, ?);";
    const std::string insertExercise = "INSERT INTO " + exercisesTable +
                                       " (block_id, position, name, exercise_type, sets, reps, "
                                       "weight_lbs, distance, distance_unit, duration_seconds, "
                                       "rest_seconds) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    for (size_t bi = 0; bi < blocks.size(); ++bi) {
        const auto& block = blocks[bi];
        sqlite3_stmt* stmtBlock = nullptr;
        if (sqlite3_prepare_v2(db, insertBlock.c_str(), -1, &stmtBlock, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_int(stmtBlock, 1, parentId);
        sqlite3_bind_int(stmtBlock, 2, static_cast<int>(bi));
        const std::string blockTypeStr = blockTypeToString(block.type);
        sqlite3_bind_text(stmtBlock, 3, blockTypeStr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtBlock, 4, block.rounds <= 0 ? 1 : block.rounds);
        bindNullableInt(stmtBlock, 5, block.rest_seconds, block.rest_seconds > 0);
        if (sqlite3_step(stmtBlock) != SQLITE_DONE) {
            sqlite3_finalize(stmtBlock);
            return false;
        }
        sqlite3_finalize(stmtBlock);
        int blockId = static_cast<int>(sqlite3_last_insert_rowid(db));

        for (size_t ei = 0; ei < block.exercises.size(); ++ei) {
            const auto& ex = block.exercises[ei];
            sqlite3_stmt* stmtEx = nullptr;
            if (sqlite3_prepare_v2(db, insertExercise.c_str(), -1, &stmtEx, nullptr) != SQLITE_OK) {
                return false;
            }
            sqlite3_bind_int(stmtEx, 1, blockId);
            sqlite3_bind_int(stmtEx, 2, static_cast<int>(ei));
            sqlite3_bind_text(stmtEx, 3, ex.name.c_str(), -1, SQLITE_TRANSIENT);
            const std::string typeStr = exerciseTypeToString(ex.type);
            sqlite3_bind_text(stmtEx, 4, typeStr.c_str(), -1, SQLITE_TRANSIENT);
            bindNullableInt(stmtEx, 5, ex.sets, ex.sets > 0);
            bindNullableInt(stmtEx, 6, ex.reps, ex.reps > 0);
            bindNullableDouble(stmtEx, 7, ex.weight_lbs, ex.weight_lbs > 0.0);
            bindNullableDouble(stmtEx, 8, ex.distance, ex.distance > 0.0);
            bindNullableText(stmtEx, 9, ex.distance_unit);
            bindNullableInt(stmtEx, 10, ex.duration_seconds, ex.duration_seconds > 0);
            bindNullableInt(stmtEx, 11, ex.rest_seconds, ex.rest_seconds > 0);

            if (sqlite3_step(stmtEx) != SQLITE_DONE) {
                sqlite3_finalize(stmtEx);
                return false;
            }
            sqlite3_finalize(stmtEx);
        }
    }
    return true;
}

bool insertBlocksAndExercises(sqlite3* db, int workoutId,
                              const std::vector<WorkoutBlock>& blocks) {
    return insertBlocksAndExercisesGeneric(db, workoutId, blocks, "workout_blocks", "workout_id",
                                           "workout_exercises");
}

void loadBlocksAndExercises(sqlite3* db, int parentId, std::vector<WorkoutBlock>& outBlocks,
                            const std::string& blocksTable, const std::string& parentFkCol,
                            const std::string& exercisesTable) {
    const std::string blocksQuery =
        "SELECT id, position, block_type, rounds, rest_seconds FROM " + blocksTable + " WHERE " +
        parentFkCol + " = ? ORDER BY position ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, blocksQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, parentId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        WorkoutBlock b;
        b.id = sqlite3_column_int(stmt, 0);
        b.position = sqlite3_column_int(stmt, 1);
        if (const char* bt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))) {
            b.type = blockTypeFromString(bt);
        }
        b.rounds = sqlite3_column_int(stmt, 3);
        b.rest_seconds =
            sqlite3_column_type(stmt, 4) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt, 4);
        outBlocks.push_back(std::move(b));
    }
    sqlite3_finalize(stmt);

    for (auto& b : outBlocks) {
        const std::string exQuery =
            "SELECT id, position, name, exercise_type, sets, reps, weight_lbs, "
            "       distance, distance_unit, duration_seconds, rest_seconds "
            "FROM " +
            exercisesTable + " WHERE block_id = ? ORDER BY position ASC;";
        sqlite3_stmt* exStmt = nullptr;
        if (sqlite3_prepare_v2(db, exQuery.c_str(), -1, &exStmt, nullptr) != SQLITE_OK) continue;
        sqlite3_bind_int(exStmt, 1, b.id);
        while (sqlite3_step(exStmt) == SQLITE_ROW) {
            WorkoutExercise e;
            e.id = sqlite3_column_int(exStmt, 0);
            e.position = sqlite3_column_int(exStmt, 1);
            if (const char* n = reinterpret_cast<const char*>(sqlite3_column_text(exStmt, 2))) {
                e.name = n;
            }
            if (const char* t = reinterpret_cast<const char*>(sqlite3_column_text(exStmt, 3))) {
                e.type = exerciseTypeFromString(t);
            }
            e.sets =
                sqlite3_column_type(exStmt, 4) == SQLITE_NULL ? 0 : sqlite3_column_int(exStmt, 4);
            e.reps =
                sqlite3_column_type(exStmt, 5) == SQLITE_NULL ? 0 : sqlite3_column_int(exStmt, 5);
            e.weight_lbs = sqlite3_column_type(exStmt, 6) == SQLITE_NULL
                               ? 0.0
                               : sqlite3_column_double(exStmt, 6);
            e.distance = sqlite3_column_type(exStmt, 7) == SQLITE_NULL
                             ? 0.0
                             : sqlite3_column_double(exStmt, 7);
            if (const char* u = reinterpret_cast<const char*>(sqlite3_column_text(exStmt, 8))) {
                e.distance_unit = u;
            }
            e.duration_seconds =
                sqlite3_column_type(exStmt, 9) == SQLITE_NULL ? 0 : sqlite3_column_int(exStmt, 9);
            e.rest_seconds = sqlite3_column_type(exStmt, 10) == SQLITE_NULL
                                 ? 0
                                 : sqlite3_column_int(exStmt, 10);
            b.exercises.push_back(std::move(e));
        }
        sqlite3_finalize(exStmt);
    }
}

}  // namespace

bool WorkoutsRepository::addWorkout(Workout& workout) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;

    d_conn->executeQuery("BEGIN TRANSACTION;");

    const std::string insertWorkout =
        "INSERT INTO workouts (name, performed_on, duration_seconds, notes, created_at) "
        "VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insertWorkout.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    bindNullableText(stmt, 1, workout.name);
    sqlite3_bind_text(stmt, 2, workout.performed_on.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, workout.duration_seconds);
    bindNullableText(stmt, 4, workout.notes);
    sqlite3_bind_int64(stmt, 5, workout.created_at);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmt);
    workout.id = static_cast<int>(sqlite3_last_insert_rowid(db));

    if (!insertBlocksAndExercises(db, workout.id, workout.blocks)) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }

    d_conn->executeQuery("COMMIT;");
    return true;
}

bool WorkoutsRepository::updateWorkout(const Workout& workout) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;

    d_conn->executeQuery("BEGIN TRANSACTION;");

    {
        const std::string delBlocks = "DELETE FROM workout_blocks WHERE workout_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, delBlocks.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_bind_int(stmt, 1, workout.id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
    }

    {
        const std::string updateRow =
            "UPDATE workouts SET name = ?, performed_on = ?, duration_seconds = ?, notes = ? "
            "WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, updateRow.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        bindNullableText(stmt, 1, workout.name);
        sqlite3_bind_text(stmt, 2, workout.performed_on.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, workout.duration_seconds);
        bindNullableText(stmt, 4, workout.notes);
        sqlite3_bind_int(stmt, 5, workout.id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
    }

    if (!insertBlocksAndExercises(db, workout.id, workout.blocks)) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }

    d_conn->executeQuery("COMMIT;");
    return true;
}

bool WorkoutsRepository::deleteWorkout(int id) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;
    const std::string del = "DELETE FROM workouts WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, del.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

Workout WorkoutsRepository::getWorkout(int id) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    Workout out;
    sqlite3* db = d_conn->raw();
    if (!db) return out;

    {
        const std::string q =
            "SELECT id, name, performed_on, duration_seconds, notes, created_at "
            "FROM workouts WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            out.id = sqlite3_column_int(stmt, 0);
            if (const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))) {
                out.name = n;
            }
            if (const char* d = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))) {
                out.performed_on = d;
            }
            out.duration_seconds = sqlite3_column_int(stmt, 3);
            if (const char* notes = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))) {
                out.notes = notes;
            }
            out.created_at = sqlite3_column_int64(stmt, 5);
        }
        sqlite3_finalize(stmt);
    }

    if (out.id == 0) return out;

    loadBlocksAndExercises(db, out.id, out.blocks, "workout_blocks", "workout_id",
                           "workout_exercises");

    return out;
}

std::vector<WorkoutSummary> WorkoutsRepository::listWorkouts() {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    std::vector<WorkoutSummary> out;
    sqlite3* db = d_conn->raw();
    if (!db) return out;

    const std::string q =
        "SELECT w.id, w.name, w.performed_on, w.duration_seconds, "
        "       COUNT(e.id) AS exercise_count "
        "FROM workouts w "
        "LEFT JOIN workout_blocks b ON b.workout_id = w.id "
        "LEFT JOIN workout_exercises e ON e.block_id = b.id "
        "GROUP BY w.id "
        "ORDER BY w.performed_on DESC, w.created_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        WorkoutSummary s;
        s.id = sqlite3_column_int(stmt, 0);
        if (const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))) {
            s.name = n;
        }
        if (const char* d = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))) {
            s.performed_on = d;
        }
        s.duration_seconds = sqlite3_column_int(stmt, 3);
        s.exercise_count = sqlite3_column_int(stmt, 4);
        out.push_back(std::move(s));
    }
    sqlite3_finalize(stmt);
    return out;
}

bool WorkoutsRepository::addTemplate(WorkoutTemplate& tmpl) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;
    if (tmpl.name.empty()) return false;

    d_conn->executeQuery("BEGIN TRANSACTION;");

    const std::string insert = "INSERT INTO workout_templates (name, created_at) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insert.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(stmt, 1, tmpl.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, tmpl.created_at);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmt);
    tmpl.id = static_cast<int>(sqlite3_last_insert_rowid(db));

    if (!insertBlocksAndExercisesGeneric(db, tmpl.id, tmpl.blocks, "template_blocks",
                                         "template_id", "template_exercises")) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }

    d_conn->executeQuery("COMMIT;");
    return true;
}

bool WorkoutsRepository::updateTemplate(const WorkoutTemplate& tmpl) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;
    if (tmpl.name.empty()) return false;

    d_conn->executeQuery("BEGIN TRANSACTION;");

    {
        const std::string delBlocks = "DELETE FROM template_blocks WHERE template_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, delBlocks.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_bind_int(stmt, 1, tmpl.id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
    }

    {
        const std::string upd = "UPDATE workout_templates SET name = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, upd.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_bind_text(stmt, 1, tmpl.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, tmpl.id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
    }

    if (!insertBlocksAndExercisesGeneric(db, tmpl.id, tmpl.blocks, "template_blocks",
                                         "template_id", "template_exercises")) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }

    d_conn->executeQuery("COMMIT;");
    return true;
}

bool WorkoutsRepository::deleteTemplate(int id) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;
    const std::string del = "DELETE FROM workout_templates WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, del.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

WorkoutTemplate WorkoutsRepository::getTemplate(int id) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    WorkoutTemplate out;
    sqlite3* db = d_conn->raw();
    if (!db) return out;

    const std::string q = "SELECT id, name, created_at FROM workout_templates WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out.id = sqlite3_column_int(stmt, 0);
        if (const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))) {
            out.name = n;
        }
        out.created_at = sqlite3_column_int64(stmt, 2);
    }
    sqlite3_finalize(stmt);
    if (out.id == 0) return out;

    loadBlocksAndExercises(db, out.id, out.blocks, "template_blocks", "template_id",
                           "template_exercises");
    return out;
}

std::vector<WorkoutTemplateSummary> WorkoutsRepository::listTemplates() {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    std::vector<WorkoutTemplateSummary> out;
    sqlite3* db = d_conn->raw();
    if (!db) return out;

    const std::string q =
        "SELECT t.id, t.name, COUNT(e.id) AS exercise_count "
        "FROM workout_templates t "
        "LEFT JOIN template_blocks b ON b.template_id = t.id "
        "LEFT JOIN template_exercises e ON e.block_id = b.id "
        "GROUP BY t.id "
        "ORDER BY t.name ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        WorkoutTemplateSummary s;
        s.id = sqlite3_column_int(stmt, 0);
        if (const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))) {
            s.name = n;
        }
        s.exercise_count = sqlite3_column_int(stmt, 2);
        out.push_back(std::move(s));
    }
    sqlite3_finalize(stmt);
    return out;
}

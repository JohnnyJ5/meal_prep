#!/usr/bin/env python3
import sqlite3
import os

def cleanup():
    db_path = 'meals.db'
    if not os.path.exists(db_path):
        # Try finding it in the parent directory if not in CWD
        if os.path.exists('../meals.db'):
            db_path = '../meals.db'
        else:
            print(f"Database {db_path} not found.")
            return

    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys to ensure CASCADE works
        cursor.execute("PRAGMA foreign_keys = ON;")
        
        # Define meals to cleanup
        test_meals = ['auto-test-pasta']
        
        deleted_count = 0
        for meal in test_meals:
            cursor.execute("DELETE FROM meals WHERE name = ?", (meal,))
            if cursor.rowcount > 0:
                deleted_count += cursor.rowcount
            
        conn.commit()
        print(f"Cleanup successful. Removed {deleted_count} test meals.")
        conn.close()
    except Exception as e:
        print(f"An error occurred during cleanup: {e}")

if __name__ == "__main__":
    cleanup()

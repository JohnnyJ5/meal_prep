#!/usr/bin/env python3
import sqlite3
import argparse
import sys

def list_tables(cursor):
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
    tables = cursor.fetchall()
    if not tables:
        print("No tables found in the database.")
    else:
        print("Tables in the database:")
        for table in tables:
            print(f"  - {table[0]}")

def dump_table(cursor, table_name):
    try:
        # Get column names first
        cursor.execute(f"PRAGMA table_info({table_name});")
        columns = [col[1] for col in cursor.fetchall()]
        
        cursor.execute(f"SELECT * FROM {table_name};")
        rows = cursor.fetchall()
        
        if not rows:
            print(f"Table '{table_name}' is empty or does not exist.")
            return

        print(f"Contents of table '{table_name}':")
        header = " | ".join(columns)
        print(header)
        print("-" * len(header))
        for row in rows:
            print(" | ".join(str(item) for item in row))
            
    except sqlite3.OperationalError as e:
        print(f"Error accessing table '{table_name}': {e}")

def main():
    parser = argparse.ArgumentParser(description="Dump SQLite database contents")
    parser.add_argument("db_file", help="Path to the SQLite database file", nargs='?', default="meals.db")
    parser.add_argument("-l", "--list", action="store_true", dest="list_tables", help="List all table names")
    parser.add_argument("-d", "--dump", metavar="TABLENAME", dest="dump_table", help="Dump the contents of the specified table")

    args = parser.parse_args()

    if not args.list_tables and not args.dump_table:
        parser.print_help()
        sys.exit(1)

    try:
        conn = sqlite3.connect(args.db_file)
        cursor = conn.cursor()

        if args.list_tables:
            list_tables(cursor)
        
        if args.dump_table:
            if args.list_tables:
                print()
            dump_table(cursor, args.dump_table)

        conn.close()

    except sqlite3.Error as e:
        print(f"Database error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()

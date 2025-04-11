#include "smart_home_energy_monitor.h"
#include "logger.h"


sqlite3 *db;

void get_current_timestamp(char *buffer, size_t buffer_size)
{
    time_t now;
    struct tm *tm_info;

    time(&now);
    tm_info = localtime(&now);

    strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S", tm_info);
}

int initialize_db()
{
	char *err_msg = 0;
	int rc;

	LOG_MSG("Initializing database");

	rc = sqlite3_open(SQL_DB, &db);

	if (rc != SQLITE_OK) {
		LOG_MSG("Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}

	// Create table for power readings
	const char *sql_create_readings = "CREATE TABLE IF NOT EXISTS power_readings (" \
				     "id INTEGER PRIMARY KEY AUTOINCREMENT," \
				     "device_id TEXT NOT NULL," \
				     "timestamp TEXT NOT NULL," \
				     "power_watts REAL);";

	rc = sqlite3_exec(db, sql_create_readings, 0, 0, &err_msg);

	if (rc != SQLITE_OK) {
		LOG_MSG("SQL exec error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return -1;
	}

	//sqlite3_close(db);
	LOG_MSG("Database initialized successfully");
	return 0;
}
/*
// Create table for aggregated hourly data
const char *sql_create_hourly = "CREATE TABLE IF NOT EXISTS hourly_aggregates ("
			   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
			   "device_id TEXT NOT NULL,"
			   "hour_timestamp TEXT NOT NULL,"
			   "avg_power REAL,"
			   "max_power REAL,"
			   "min_power REAL,"
			   "readings_count INTEGER);";

rc = sqlite3_exec(db, sql_create_hourly, 0, 0, &err_msg);

if (rc != SQLITE_OK) {
fprintf(stderr, "SQL error: %s\n", err_msg);
sqlite3_free(err_msg);
sqlite3_close(db);
return 1;
*/

int store_reading(const char *device_id, double power_reading)
{
	sqlite3_stmt *stmt;
	char timestamp[64];
	int rc;

	// Get current timestamp
	get_current_timestamp(timestamp, sizeof(timestamp));

	// Prepare the SQL statement
	const char *sql = "INSERT INTO power_readings "
		     "(device_id, timestamp, power_watts) "
		     "VALUES (?, ?, ?)";

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		LOG_MSG("Failed to prepare statement: %s", sqlite3_errmsg(db));
		return -1;
	}

	// Bind parameters
	sqlite3_bind_text(stmt, 1, device_id, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, timestamp, -1, SQLITE_STATIC);
	sqlite3_bind_double(stmt, 3, power_reading);

	// Execute the statement
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		LOG_MSG("Execution failed: %s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return -1;
	}

	sqlite3_finalize(stmt);
	return 0;
}

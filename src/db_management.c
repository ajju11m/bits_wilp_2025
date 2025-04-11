#include "smart_home_energy_monitor.h"
#include "logger.h"

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
		LOG_MSG("SQL power_readings table exec error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return -1;
	}

	// Create table for aggregated 10 min data
	const char *sql_create_aggregate = "CREATE TABLE IF NOT EXISTS aggregated_readings ("
				   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
				   "device_id TEXT NOT NULL,"
				   "timestamp TEXT NOT NULL,"
				   "avg_power REAL,"
				   "max_power REAL,"
				   "min_power REAL,"
				   "readings_count INTEGER);";

	rc = sqlite3_exec(db, sql_create_aggregate, 0, 0, &err_msg);

	if (rc != SQLITE_OK) {
		LOG_MSG("SQL aggregated_readings table exec error : %s", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return -1;
	}

	//sqlite3_close(db);
	LOG_MSG("Database initialized successfully");
	return 0;
}

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

int store_five_minute_aggregate(const char *device_id, const struct tm *tm_interval)
{
	sqlite3_stmt *stmt;
	int rc;
	char end_timestamp[32];

	strftime(end_timestamp, sizeof(end_timestamp), "%Y-%m-%dT%H:%M:%S", tm_interval);

	// First, query for aggregated data from the Interval
	const char *sql_query = "SELECT AVG(power_watts), MAX(power_watts), MIN(power_watts), COUNT(*) "
			       "FROM power_readings "
			       "WHERE device_id = ? AND timestamp >= ? AND timestamp < ?";

	rc = sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		LOG_MSG("Failed to prepare query statement: %s", sqlite3_errmsg(db));
		return -1;
	}

	// Bind query parameters
	sqlite3_bind_text(stmt, 1, device_id, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, start_timestamp, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, end_timestamp, -1, SQLITE_STATIC);

	// Execute query
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW) {
		LOG_MSG("No data found for aggregated data");
		sqlite3_finalize(stmt);
		return -1;
	}

	// Extract results
	double avg_power = sqlite3_column_double(stmt, 0);
	double max_power = sqlite3_column_double(stmt, 1);
	double min_power = sqlite3_column_double(stmt, 2);
	int readings_count = sqlite3_column_int(stmt, 3);

	sqlite3_finalize(stmt);

	// Skip if no readings
	if (readings_count == 0) {
		return 0;
	}

	// Prepare the insert statement
	const char *sql_insert = "INSERT INTO aggregated_readings "
			    "(device_id, timestamp, avg_power, max_power, min_power, readings_count) "
			    "VALUES (?, ?, ?, ?, ?, ?)";

	rc = sqlite3_prepare_v2(db, sql_insert, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		LOG_MSG("Failed to prepare insert statement: %s", sqlite3_errmsg(db));
		return -1;
	}

	// Bind insert parameters
	sqlite3_bind_text(stmt, 1, device_id, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, end_timestamp, -1, SQLITE_STATIC);
	sqlite3_bind_double(stmt, 3, avg_power);
	sqlite3_bind_double(stmt, 4, max_power);
	sqlite3_bind_double(stmt, 5, min_power);
	sqlite3_bind_int(stmt, 6, readings_count);

	// Execute insert
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		LOG_MSG("Failed to insert aggregated_readings: %s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return -1;
	}

	sqlite3_finalize(stmt);
	LOG_MSG("Stored Aggregated Data for %s at %s", device_id, end_timestamp);
	return 0;
}

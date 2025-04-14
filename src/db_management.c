#include "smart_home_energy_monitor.h"

void get_current_timestamp(char *buffer, size_t buffer_size)
{
    time_t now;
    struct tm *tm_info;

    time(&now);
    tm_info = localtime(&now);

    strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S", tm_info);
}

/**
 * initialize_db() - Initializes the SQLite database and creates required tables.
 *
 * This function opens a connection to the database defined by the macro `SQL_DB`
 * and ensures that two tables, `power_readings` and `aggregated_readings`,
 * are created if they do not already exist.
 *
 * The `power_readings` table stores individual power readings sampled from
 * each appliance, while the `aggregated_readings` table is intended to store
 * five-minute aggregated power data such as average, maximum, minimum values,
 * and count of readings.
 *
 * Return: 0 on successful initialization of the database and tables or -1 is returned.
 */
int initialize_db()
{
	char *err_msg = 0;
	int ret;

	LOG_MSG("Initializing database");

	ret = sqlite3_open(SQL_DB, &db);

	if (ret != SQLITE_OK) {
		LOG_MSG("Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}

	// Create table for power readings
	const char *sql_create_readings = "CREATE TABLE IF NOT EXISTS power_readings (" \
				     "id INTEGER PRIMARY KEY AUTOINCREMENT," \
				     "device_id TEXT NOT NULL," \
				     "timestamp TEXT NOT NULL," \
				     "power_watts INTEGER);";

	ret = sqlite3_exec(db, sql_create_readings, 0, 0, &err_msg);

	if (ret != SQLITE_OK) {
		LOG_MSG("SQL power_readings table exec error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return -1;
	}

	// Create table for 5min aggregated data
	const char *sql_create_aggregate = "CREATE TABLE IF NOT EXISTS aggregated_readings ("
				   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
				   "device_id TEXT NOT NULL,"
				   "timestamp TEXT NOT NULL,"
				   "avg_power INTEGER,"
				   "max_power INTEGER,"
				   "min_power INTEGER,"
				   "readings_count INTEGER);";

	ret = sqlite3_exec(db, sql_create_aggregate, 0, 0, &err_msg);

	if (ret != SQLITE_OK) {
		LOG_MSG("SQL aggregated_readings table exec error : %s", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return -1;
	}

	LOG_MSG("Database initialized successfully");

	return 0;
}

/**
 * store_reading() - Stores a single power reading into the database.
 *
 * @sampled_data: A populated sensor_data structure containing device name,
 *                timestamp, and power reading to be persisted into the
 *                power_readings table.
 *
 * Return: 0 on success or -1 to indicate failure.
 */

int store_reading(struct sensor_data sampled_data)
{
	sqlite3_stmt *stmt;
	int ret;

	const char *sql = "INSERT INTO power_readings "
		     "(device_id, timestamp, power_watts) "
		     "VALUES (?, ?, ?)";

	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
		LOG_MSG("Failed to prepare statement: %s", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_text(stmt, 1, sampled_data.device_name, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, sampled_data.timestamp, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 3, sampled_data.power);

	ret = sqlite3_step(stmt);
	if (ret != SQLITE_DONE) {
		LOG_MSG("Execution failed: %s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return -1;
	}

	sqlite3_finalize(stmt);

	return 0;
}

/**
 * store_five_minute_aggregate() - Stores 5-minute aggregated sensor data into the database.
 *
 * @device_id: Name/identifier of the appliance whose data is being aggregated.
 *
 * @tm_interval: A pointer to a struct tm representing the end time of the 5-minute interval.
 *
 * Return: 0 on success, including if there are no readings in the interval (i.e., nothing to aggregate)
 * or -1 upon failures
 */
int store_five_minute_aggregate(const char *device_id, const struct tm *tm_interval)
{
	sqlite3_stmt *stmt;
	int avg_power, max_power, min_power, readings_count, ret;
	char end_timestamp[32];
	const char *sql_query = "SELECT AVG(power_watts), MAX(power_watts), MIN(power_watts), COUNT(*) "
			       "FROM power_readings "
			       "WHERE device_id = ? AND timestamp >= ? AND timestamp < ?";

	ret = sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
		LOG_MSG("Failed to prepare query statement: %s", sqlite3_errmsg(db));
		return -1;
	}

	strftime(end_timestamp, sizeof(end_timestamp), "%Y-%m-%dT%H:%M:%S", tm_interval);

	sqlite3_bind_text(stmt, 1, device_id, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, start_timestamp, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, end_timestamp, -1, SQLITE_STATIC);

	ret = sqlite3_step(stmt);
	if (ret != SQLITE_ROW) {
		LOG_MSG("No data found for aggregated data");
		sqlite3_finalize(stmt);
		return -1;
	}

	avg_power = sqlite3_column_int(stmt, 0);
	max_power = sqlite3_column_int(stmt, 1);
	min_power = sqlite3_column_int(stmt, 2);
	readings_count = sqlite3_column_int(stmt, 3);

	sqlite3_finalize(stmt);

	if (readings_count == 0) {
		return 0;
	}

	const char *sql_insert = "INSERT INTO aggregated_readings "
			    "(device_id, timestamp, avg_power, max_power, min_power, readings_count) "
			    "VALUES (?, ?, ?, ?, ?, ?)";

	ret = sqlite3_prepare_v2(db, sql_insert, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
		LOG_MSG("Failed to prepare insert statement: %s", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_text(stmt, 1, device_id, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, end_timestamp, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 3, avg_power);
	sqlite3_bind_int(stmt, 4, max_power);
	sqlite3_bind_int(stmt, 5, min_power);
	sqlite3_bind_int(stmt, 6, readings_count);

	ret = sqlite3_step(stmt);
	if (ret != SQLITE_DONE) {
		LOG_MSG("Failed to insert aggregated_readings: %s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return -1;
	}

	sqlite3_finalize(stmt);
	LOG_MSG("Stored Aggregated Data for %s at %s", device_id, end_timestamp);

	return 0;
}

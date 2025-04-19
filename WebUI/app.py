from flask import Flask, render_template, request
import sqlite3

DB_PATH = "../data/energy_monitor.db"

app = Flask(__name__)

# Function to connect to the SQLite database
def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def format_timestamp_to_time(timestamp):
    if isinstance(timestamp, str):
        # For string timestamps like '2025-04-15T14:01:04'
        if 'T' in timestamp:
            return timestamp.split('T')[1]
        else:
            # Handle other string formats if needed
            return timestamp
    else:
        # For datetime objects
        return timestamp.strftime('%H:%M:%S')

@app.route('/')
def index():
    # Default active tab is 'real-time-tab'
    active_tab = request.args.get('tab', 'real-time-tab')
    return render_template('index.html', data=None, active_tab=active_tab)

@app.route('/fetch_real_time_data', methods=['POST'])
def fetch_real_time_data():
    device_id = request.form.get('device_id')
    date = request.form.get('date')

    # Fetch real-time data for the selected appliance and date
    conn = get_db_connection()

    if device_id == "All":
        query = """
        SELECT device_id, timestamp, power_watts
        FROM power_readings
        WHERE date(timestamp) = ?
        ORDER BY timestamp DESC
        """
        result = conn.execute(query, (date,)).fetchall()
    else:
        query = """
        SELECT device_id, timestamp, power_watts
        FROM power_readings
        WHERE device_id = ? AND date(timestamp) = ?
        ORDER BY timestamp DESC
        """
        result = conn.execute(query, (device_id, date)).fetchall()

    conn.close()

    # Format the timestamp to only show time (HH:MM:SS)
    formatted_result = []
    for row in result:
        # Assuming row is a dict or a sqlite3.Row
        formatted_row = dict(row) if hasattr(row, 'keys') else {'device_id': row[0], 'timestamp': row[1], 'power_watts': row[2]}
        formatted_row['timestamp'] = format_timestamp_to_time(formatted_row['timestamp'])
        formatted_result.append(formatted_row)

    print("Fetched real-time data:", formatted_result)

    return render_template(
        'index.html',
        data=formatted_result if formatted_result else None,
        device_id=device_id,
        date=date,
        active_tab='real-time-tab'
    )

@app.route('/fetch_aggregate_data', methods=['POST'])
def fetch_aggregate_data():
    device_id = request.form.get('device_id')
    date = request.form.get('date')
    timestamp = request.form.get('timestamp')

    print("requested data", device_id, date, timestamp)

    # Query for the exact timestamp first
    conn = get_db_connection()
    query = """
    SELECT device_id, timestamp, min_power, avg_power, max_power
    FROM aggregated_readings
    WHERE device_id= ? AND date(timestamp) = ? AND strftime('%H:%M', timestamp) = ?
    """
    result = conn.execute(query, (device_id, date, timestamp)).fetchone()

    if result is None:
        # If no exact match, query for the nearest earlier timestamp
        query = """
        SELECT device_id, timestamp, min_power, avg_power, max_power
        FROM aggregated_readings
        WHERE device_id = ? AND date(timestamp) = ? AND strftime('%H:%M', timestamp) <= ?
        ORDER BY timestamp DESC
        LIMIT 1
        """
        result = conn.execute(query, (device_id, date, timestamp)).fetchone()

        # If a result is found, mark it as a nearest match
        if result:
            result = dict(result)
            result['exact_match'] = False
    else:
        # If an exact match is found, mark it as an exact match
        result = dict(result)
        result['exact_match'] = True

    conn.close()

    if result:
        del result['timestamp']

    print("Fetched data:", result)

    # Return the result and ensure the active tab is preserved
    return render_template('index.html', 
                           data=[result] if result else None,
                           device_id=device_id, 
                           date=date, 
                           timestamp=timestamp, 
                           active_tab='aggregate-tab')

if __name__ == '__main__':
    app.run(debug=True)

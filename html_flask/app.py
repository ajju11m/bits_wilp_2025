from flask import Flask, render_template, request
import sqlite3

app = Flask(__name__)

# Function to connect to the SQLite database
def get_db_connection():
    conn = sqlite3.connect('energy_monitor 1.db')
    conn.row_factory = sqlite3.Row
    return conn

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
    query = """
    SELECT device_id, timestamp, power_watts
    FROM power_readings
    WHERE device_id = ? AND date(timestamp) = ?
    """
    result = conn.execute(query, (device_id, date)).fetchall()
    conn.close()
    
    print("Fetched data:", result)


    # Return the result and ensure the active tab is preserved
    return render_template(
    'index.html',
    data=result,
    device_id=device_id,
    date=date,
    active_tab='real-time-tab'
)



@app.route('/fetch_aggregate_data', methods=['POST'])
def fetch_aggregate_data():
    device_id = request.form.get('device_id')
    date = request.form.get('date')
    timestamp = request.form.get('timestamp')

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

from flask import Flask, g, request, jsonify
import sqlite3
from urllib.parse import unquote



app = Flask(__name__)

DATABASE = 'SQLite/ScanLinkDataBase.db'

def get_db():
    # Opening a new database connection
    if 'db' not in g:
        g.db = sqlite3.connect(DATABASE)
    return g.db  # Return the database connection

@app.teardown_appcontext
def close_db(error):
    # Close the database connection at the end of the request
    db = g.pop('db', None)
    if db is not None:
        db.close()

@app.route('/verify', methods=['GET'])
def verify_qr():
    # Extract 'id' and 'name' from query parameters
    qr_id = request.args.get("id")
    qr_name = request.args.get("name")
    
    if not qr_id or not qr_name:
        return jsonify({"status": "error", "message": "Missing 'id' or 'name' in query parameters"}), 400
    
    from urllib.parse import unquote
    
    db = get_db()
    cursor = db.cursor()
    cursor.execute("SELECT * FROM test WHERE id = ? AND name = ?", (qr_id, qr_name))
    result = cursor.fetchone()

    # Verify if a result was found
    if result:
        return "verified"
    else:
        return "unverified"

@app.route('/')
def index():
    # Example of using the database
    db = get_db()
    cursor = db.cursor()
    cursor.execute('SELECT * FROM test')  # Query the table
    test = cursor.fetchall()
    return str(test)  # Return the data as a string

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)

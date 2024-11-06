ScanLink
Overview
ScanLink is a QR code authenticator designed to verify identities by cross-referencing with a database. This project focuses on efficient and secure identity verification, making use of QR scanning technology and a backend system for real-time checks.

Features
QR Code Scanning: Reads and decodes QR codes containing user details.
Database Integration: Utilizes SQLite for identity verification.
Status Indicators: Mini LED indicators (green for verified, red for unverified).
Installation
Clone the repository:
bash
Copy code
git clone <repository-url>
Navigate to the project directory:
bash
Copy code
cd ScanLink
Install the required dependencies:
bash
Copy code
pip install -r requirements.txt
Usage
Start the application:
bash
Copy code
python app.py
Scan a QR code using the interface to verify identity against the SQLite database.
Technical Stack
Frontend: Basic web interface (or specify further if applicable)
Backend: Python (Flask 3.0.3)
Database: SQLite
Development Environment: Visual Studio Code / GitHub Desktop for version control

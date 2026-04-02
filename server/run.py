"""TianShu - IoT Development Platform"""
import os
from dotenv import load_dotenv

load_dotenv()

from app import create_app
from app.extensions import socketio

app = create_app()

if __name__ == '__main__':
    host = os.getenv('HOST', '0.0.0.0')
    port = int(os.getenv('PORT', 5000))
    debug = os.getenv('DEBUG', 'True').lower() == 'true'

    socketio.run(app, host=host, port=port, debug=debug)

"""Flask Extensions"""
from flask_sqlalchemy import SQLAlchemy
from flask_jwt_extended import JWTManager
from flask_cors import CORS
from flask_migrate import Migrate
from flask_socketio import SocketIO

db = SQLAlchemy()
jwt = JWTManager()
cors = CORS(supports_credentials=True, resources={r"/api/*": {"origins": "*"}})
migrate = Migrate()
socketio = SocketIO(
    cors_allowed_origins="*",
    ping_timeout=60,
    ping_interval=25,
    async_mode='eventlet'
)

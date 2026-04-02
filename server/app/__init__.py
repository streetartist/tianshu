"""TianShu - Flask Application Factory"""
import os
from flask import Flask, jsonify
from .config import config
from .extensions import db, jwt, cors, migrate, socketio


def create_app(config_name=None):
    """Create Flask application"""
    if config_name is None:
        config_name = os.getenv('FLASK_ENV', 'development')

    app = Flask(__name__)
    app.config.from_object(config[config_name])

    # Initialize extensions
    db.init_app(app)
    jwt.init_app(app)
    cors.init_app(app)
    migrate.init_app(app, db)
    socketio.init_app(app)

    # Register blueprints
    register_blueprints(app)

    # Register WebSocket events
    from .websocket import register_events
    register_events(socketio)

    # Root routes
    @app.route('/')
    def index():
        return jsonify({
            'name': 'TianShu',
            'name_cn': '天枢',
            'description': 'IoT Development Platform',
            'version': '1.0.0',
            'api': '/api/v1'
        })

    @app.route('/health')
    def health():
        return jsonify({'status': 'ok'})

    # Create tables
    with app.app_context():
        db.create_all()

    # Initialize scheduler (only in non-testing mode)
    if config_name != 'testing':
        from .scheduler import init_scheduler
        init_scheduler(app)

    return app


def register_blueprints(app):
    """Register API blueprints"""
    from .api.v1 import api_v1
    app.register_blueprint(api_v1, url_prefix='/api/v1')

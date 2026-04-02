"""Authentication API"""
from flask import request, jsonify
from flask_jwt_extended import create_access_token, jwt_required, get_jwt_identity
from . import api_v1
from ...extensions import db
from ...models import User


@api_v1.route('/auth/register', methods=['POST'])
def register():
    """Register new user"""
    data = request.get_json()

    if not data or not data.get('username') or not data.get('password'):
        return jsonify({'code': 1001, 'message': 'Missing required fields'}), 400

    if User.query.filter_by(username=data['username']).first():
        return jsonify({'code': 1001, 'message': 'Username already exists'}), 400

    user = User(
        username=data['username'],
        email=data.get('email', f"{data['username']}@local")
    )
    user.set_password(data['password'])

    db.session.add(user)
    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'User registered successfully',
        'data': user.to_dict()
    })


@api_v1.route('/auth/login', methods=['POST'])
def login():
    """User login"""
    data = request.get_json()

    if not data or not data.get('username') or not data.get('password'):
        return jsonify({'code': 1001, 'message': 'Missing credentials'}), 400

    user = User.query.filter_by(username=data['username']).first()

    if not user or not user.check_password(data['password']):
        return jsonify({'code': 1002, 'message': 'Invalid credentials'}), 401

    if not user.is_active:
        return jsonify({'code': 1002, 'message': 'Account disabled'}), 401

    token = create_access_token(identity=str(user.id))

    return jsonify({
        'code': 0,
        'message': 'Login successful',
        'data': {
            'token': token,
            'user': user.to_dict()
        }
    })


@api_v1.route('/auth/me', methods=['GET'])
@jwt_required()
def get_current_user():
    """Get current user info"""
    user_id = get_jwt_identity()
    user = User.query.get(user_id)

    if not user:
        return jsonify({'code': 1002, 'message': 'User not found'}), 404

    return jsonify({
        'code': 0,
        'data': user.to_dict()
    })

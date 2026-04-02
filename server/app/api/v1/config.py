"""Device Configuration API"""
from flask import request, jsonify
from flask_jwt_extended import jwt_required, get_jwt_identity
from . import api_v1
from ...extensions import db
from ...models import Device, DeviceConfig


@api_v1.route('/config/pull', methods=['POST'])
def pull_config():
    """Device pulls its configuration (device auth)"""
    data = request.get_json()

    if not data or not data.get('device_id') or not data.get('secret_key'):
        return jsonify({'code': 1001, 'message': 'Missing credentials'}), 400

    device = Device.query.filter_by(
        device_id=data['device_id'],
        secret_key=data['secret_key']
    ).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    configs = DeviceConfig.query.filter_by(device_id=device.id).all()

    return jsonify({
        'code': 0,
        'data': {c.key: c.value for c in configs}
    })


@api_v1.route('/devices/<device_id>/config', methods=['GET'])
@jwt_required()
def get_device_config(device_id):
    """Get device configuration by device_id (UUID)"""
    user_id = get_jwt_identity()
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    configs = DeviceConfig.query.filter_by(device_id=device.id).all()

    return jsonify({
        'code': 0,
        'data': {c.key: c.value for c in configs}
    })


@api_v1.route('/devices/<device_id>/config', methods=['PUT'])
@jwt_required()
def update_device_config(device_id):
    """Update device configuration by device_id (UUID)"""
    user_id = get_jwt_identity()
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    data = request.get_json()
    for key, value in data.items():
        config = DeviceConfig.query.filter_by(
            device_id=device.id, key=key
        ).first()

        if config:
            config.value = value
            config.version += 1
        else:
            config = DeviceConfig(
                device_id=device.id,
                key=key,
                value=value
            )
            db.session.add(config)

    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'Config updated'
    })

"""Device Management API"""
from flask import request, jsonify
from flask_jwt_extended import jwt_required, get_jwt_identity
from datetime import datetime
from . import api_v1
from ...extensions import db
from ...models import Device, DeviceGroup, DeviceTemplate, DeviceConfig
from ...services.device_service import DeviceService


@api_v1.route('/devices', methods=['GET'])
@jwt_required()
def list_devices():
    """List all devices"""
    user_id = get_jwt_identity()
    devices = Device.query.filter_by(user_id=user_id).all()

    return jsonify({
        'code': 0,
        'data': [d.to_dict() for d in devices]
    })


@api_v1.route('/devices', methods=['POST'])
@jwt_required()
def create_device():
    """Register new device"""
    user_id = get_jwt_identity()
    data = request.get_json()

    if not data or not data.get('name') or not data.get('device_type'):
        return jsonify({'code': 1001, 'message': 'Missing required fields'}), 400

    device = Device(
        device_id=Device.generate_device_id(),
        secret_key=Device.generate_secret_key(),
        name=data['name'],
        device_type=data['device_type'],
        description=data.get('description'),
        user_id=user_id,
        tags=data.get('tags', [])
    )

    db.session.add(device)
    db.session.flush()  # 获取device.id

    # 从模板继承配置
    template = DeviceTemplate.query.filter_by(name=data['device_type']).first()
    if template:
        # 继承通信设置
        comm_settings = template.comm_settings or {}
        for key, value in comm_settings.items():
            config = DeviceConfig(device_id=device.id, key=key, value=value)
            db.session.add(config)
        # 继承自定义配置项
        config_items = template.config_items or []
        for item in config_items:
            config = DeviceConfig(
                device_id=device.id,
                key=item.get('key'),
                value=item.get('default_value', '')
            )
            db.session.add(config)

    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'Device created',
        'data': {
            **device.to_dict(),
            'secret_key': device.secret_key  # Only returned on creation
        }
    })


@api_v1.route('/devices/<device_id>', methods=['GET'])
@jwt_required()
def get_device(device_id):
    """Get device detail by device_id (UUID)"""
    user_id = int(get_jwt_identity())
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    return jsonify({
        'code': 0,
        'data': device.to_dict()
    })


@api_v1.route('/devices/<device_id>', methods=['PUT'])
@jwt_required()
def update_device(device_id):
    """Update device by device_id (UUID)"""
    user_id = get_jwt_identity()
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    data = request.get_json()
    if data.get('name'):
        device.name = data['name']
    if data.get('description') is not None:
        device.description = data['description']
    if data.get('tags') is not None:
        device.tags = data['tags']

    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'Device updated',
        'data': device.to_dict()
    })


@api_v1.route('/devices/<device_id>', methods=['DELETE'])
@jwt_required()
def delete_device(device_id):
    """Delete device by device_id (UUID)"""
    user_id = get_jwt_identity()
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    db.session.delete(device)
    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'Device deleted'
    })


@api_v1.route('/devices/<device_id>/secret', methods=['GET'])
@jwt_required()
def get_device_secret(device_id):
    """Get device secret key by device_id (UUID)"""
    user_id = get_jwt_identity()
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    return jsonify({
        'code': 0,
        'data': {
            'device_id': device.device_id,
            'secret_key': device.secret_key
        }
    })


@api_v1.route('/devices/<device_id>/secret', methods=['POST'])
@jwt_required()
def reset_device_secret(device_id):
    """Reset device secret key by device_id (UUID)"""
    user_id = get_jwt_identity()
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    device.secret_key = Device.generate_secret_key()
    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'Secret key reset',
        'data': {
            'device_id': device.device_id,
            'secret_key': device.secret_key
        }
    })


@api_v1.route('/devices/<device_id>/info', methods=['GET'])
def get_device_by_secret(device_id):
    """Get device status - secret_key auth for device apps"""
    secret_key = request.args.get('secret_key') or request.headers.get('X-Secret-Key')

    if not secret_key:
        return jsonify({'code': 1001, 'message': 'Missing secret_key'}), 400

    device = Device.query.filter_by(device_id=device_id, secret_key=secret_key).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found or invalid secret'}), 404

    return jsonify({
        'code': 0,
        'data': device.to_dict()
    })


@api_v1.route('/devices/heartbeat', methods=['POST'])
def device_heartbeat():
    """Device heartbeat - update online status"""
    data = request.get_json()
    device_id = data.get('device_id')
    secret_key = data.get('secret_key')

    if not device_id or not secret_key:
        return jsonify({'code': 1001, 'message': 'Missing credentials'}), 400

    device = DeviceService.get_device_by_id(device_id)
    if not device or device.secret_key != secret_key:
        return jsonify({'code': 2002, 'message': 'Invalid credentials'}), 401

    DeviceService.update_device_status(device, 'online')

    return jsonify({
        'code': 0,
        'message': 'Heartbeat received',
        'data': {'status': 'online'}
    })


@api_v1.route('/devices/check-status', methods=['POST'])
@jwt_required()
def check_device_status():
    """Check and update all offline devices"""
    DeviceService.check_all_device_status()

    return jsonify({
        'code': 0,
        'message': 'Status check completed'
    })


@api_v1.route('/devices/<device_id>/command', methods=['POST'])
@jwt_required()
def send_device_command(device_id):
    """Send command to device via WebSocket"""
    user_id = get_jwt_identity()
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    data = request.get_json()
    command = data.get('command')
    params = data.get('params', {})

    if not command:
        return jsonify({'code': 1001, 'message': 'Missing command'}), 400

    from ...websocket import send_command
    success, msg = send_command(device_id, command, params)

    if success:
        return jsonify({'code': 0, 'message': msg})
    else:
        return jsonify({'code': 2002, 'message': msg}), 400


@api_v1.route('/devices/online', methods=['GET'])
@jwt_required()
def get_online_devices():
    """Get list of online devices"""
    from ...websocket import get_online_devices
    online = get_online_devices()

    return jsonify({
        'code': 0,
        'data': online
    })


@api_v1.route('/devices/<device_id>/data', methods=['GET'])
def get_device_data_by_secret(device_id):
    """Get device data records - secret_key auth for device apps"""
    from ...models import DataRecord

    secret_key = request.args.get('secret_key') or request.headers.get('X-Secret-Key')

    if not secret_key:
        return jsonify({'code': 1001, 'message': 'Missing secret_key'}), 400

    device = Device.query.filter_by(device_id=device_id, secret_key=secret_key).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found or invalid secret'}), 404

    limit = request.args.get('limit', 100, type=int)
    data_type = request.args.get('data_type')

    query = DataRecord.query.filter_by(device_id=device.id)
    if data_type:
        query = query.filter_by(data_type=data_type)
    records = query.order_by(DataRecord.timestamp.desc()).limit(limit).all()

    return jsonify({
        'code': 0,
        'data': [r.to_dict() for r in records]
    })

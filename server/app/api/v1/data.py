"""Data API"""
from flask import request, jsonify
from flask_jwt_extended import jwt_required, get_jwt_identity
from datetime import datetime
from . import api_v1
from ...extensions import db, socketio
from ...models import Device, DataRecord
from ...models.data_record import format_float_values


@api_v1.route('/data/report', methods=['POST'])
def report_data():
    """Device data report endpoint"""
    data = request.get_json()

    if not data or not data.get('device_id') or not data.get('secret_key'):
        return jsonify({'code': 1001, 'message': 'Missing credentials'}), 400

    device = Device.query.filter_by(
        device_id=data['device_id'],
        secret_key=data['secret_key']
    ).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    data_type = data.get('data_type', 'sensor')
    payload = data.get('data', {})

    # 更新设备状态
    device.status = 'online'
    device.last_seen = datetime.utcnow()

    # 合并更新设备当前数据状态
    if device.current_data:
        device.current_data = {**device.current_data, **payload}
    else:
        device.current_data = payload

    # Save data record
    record = DataRecord(
        device_id=device.id,
        data_type=data_type,
        data=payload
    )

    db.session.add(record)
    db.session.commit()

    # 推送数据给订阅的App（推送合并后的完整数据）
    socketio.emit('device_data', {
        'device_id': device.device_id,
        'data': format_float_values(device.current_data),
        'data_type': data_type,
        'timestamp': record.timestamp.isoformat()
    })

    return jsonify({
        'code': 0,
        'message': 'Data received',
        'data': {'record_id': record.id}
    })


@api_v1.route('/data/<device_id>', methods=['GET'])
@jwt_required()
def get_device_data(device_id):
    """Get device data records by device_id (UUID) - JWT auth"""
    user_id = get_jwt_identity()
    device = Device.query.filter_by(device_id=device_id, user_id=user_id).first()

    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    limit = request.args.get('limit', 100, type=int)
    records = DataRecord.query.filter_by(device_id=device.id)\
        .order_by(DataRecord.timestamp.desc())\
        .limit(limit).all()

    return jsonify({
        'code': 0,
        'data': [r.to_dict() for r in records]
    })

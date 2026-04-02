"""WebSocket Events for Device Communication"""
from flask import request
from flask_socketio import emit, join_room, leave_room
from .extensions import db
from .models import Device

# 存储在线设备 {device_id: sid}
online_devices = {}


def register_events(socketio):
    """Register WebSocket event handlers"""

    @socketio.on('connect')
    def handle_connect():
        print(f'[WS] Client connected: {request.sid}')

    @socketio.on('disconnect')
    def handle_disconnect():
        sid = request.sid
        # 移除离线设备
        device_id = None
        for did, s in list(online_devices.items()):
            if s == sid:
                device_id = did
                del online_devices[did]
                break
        if device_id:
            # 广播设备离线状态
            socketio.emit('device_status', {
                'device_id': device_id,
                'status': 'offline'
            })
            print(f'[WS] Device offline: {device_id}')
        print(f'[WS] Client disconnected: {sid}')

    @socketio.on('auth')
    def handle_auth(data):
        """设备认证"""
        device_id = data.get('device_id')
        secret_key = data.get('secret_key')

        if not device_id or not secret_key:
            emit('auth_result', {'code': 1001, 'message': 'Missing credentials'})
            return

        device = Device.query.filter_by(
            device_id=device_id,
            secret_key=secret_key
        ).first()

        if not device:
            emit('auth_result', {'code': 2001, 'message': 'Invalid credentials'})
            return

        # 认证成功，加入设备房间
        join_room(f'device_{device_id}')
        online_devices[device_id] = request.sid

        # 更新设备状态
        device.status = 'online'
        db.session.commit()

        emit('auth_result', {'code': 0, 'message': 'Authenticated'})

        # 广播设备上线状态
        socketio.emit('device_status', {
            'device_id': device_id,
            'status': 'online'
        })
        print(f'[WS] Device authenticated: {device_id}')

    @socketio.on('heartbeat')
    def handle_heartbeat(data):
        """设备心跳"""
        device_id = data.get('device_id')
        if device_id in online_devices:
            emit('heartbeat_ack', {'code': 0})

    @socketio.on('cmd_result')
    def handle_cmd_result(data):
        """设备命令执行结果"""
        device_id = data.get('device_id')
        cmd_id = data.get('cmd_id')
        success = data.get('success', False)
        result = data.get('result')

        print(f'[WS] Command result from {device_id}: cmd={cmd_id}, success={success}')
        # 可以存储命令执行结果到数据库

    @socketio.on('send_command')
    def handle_send_command(data):
        """App发送命令给设备（需要验证secret_key）"""
        device_id = data.get('device_id')
        secret_key = data.get('secret_key')
        command = data.get('command')
        params = data.get('params', {})

        if not device_id or not command:
            emit('command_result', {'code': 1001, 'message': 'Missing device_id or command'})
            return

        if not secret_key:
            emit('command_result', {'code': 1001, 'message': 'Missing secret_key'})
            return

        # 验证secret_key
        device = Device.query.filter_by(device_id=device_id, secret_key=secret_key).first()
        if not device:
            emit('command_result', {'code': 2001, 'message': 'Invalid credentials'})
            return

        success, msg = send_command(device_id, command, params)
        emit('command_result', {
            'code': 0 if success else 2002,
            'message': msg,
            'device_id': device_id,
            'command': command
        })


def send_command(device_id, command, params=None):
    """向设备发送命令"""
    from .extensions import socketio

    if device_id not in online_devices:
        return False, 'Device offline'

    socketio.emit('command', {
        'command': command,
        'params': params or {}
    }, room=f'device_{device_id}')

    return True, 'Command sent'


def is_device_online(device_id):
    """检查设备是否在线"""
    return device_id in online_devices


def get_online_devices():
    """获取所有在线设备"""
    return list(online_devices.keys())


def get_online_devices_detail():
    """获取在线设备详情（用于调试）"""
    return {did: sid for did, sid in online_devices.items()}

"""Agent Tools Service"""
from ..models import Device, DataRecord, DeviceConfig
from ..extensions import db


class AgentTools:
    """Built-in tools for AI Agent"""

    AVAILABLE_TOOLS = {
        'device_list': {
            'name': 'device_list',
            'description': '获取用户的设备列表，可通过名称搜索设备，返回设备ID和名称',
            'parameters': {
                'name': {'type': 'string', 'description': '设备名称（可选，模糊搜索）', 'default': ''}
            }
        },
        'device_control': {
            'name': 'device_control',
            'description': '控制设备，发送命令',
            'parameters': {
                'device_id': {'type': 'integer', 'description': '设备ID'},
                'command': {'type': 'string', 'description': '命令名称'},
                'params': {'type': 'object', 'description': '命令参数'}
            }
        },
        'device_query': {
            'name': 'device_query',
            'description': '查询设备状态和信息',
            'parameters': {
                'device_id': {'type': 'integer', 'description': '设备ID'}
            }
        },
        'data_query': {
            'name': 'data_query',
            'description': '查询设备历史数据',
            'parameters': {
                'device_id': {'type': 'integer', 'description': '设备ID'},
                'limit': {'type': 'integer', 'description': '返回条数', 'default': 10}
            }
        },
        'config_get': {
            'name': 'config_get',
            'description': '获取设备配置',
            'parameters': {
                'device_id': {'type': 'integer', 'description': '设备ID'}
            }
        },
        'config_set': {
            'name': 'config_set',
            'description': '设置设备配置',
            'parameters': {
                'device_id': {'type': 'integer', 'description': '设备ID'},
                'key': {'type': 'string', 'description': '配置项'},
                'value': {'type': 'string', 'description': '配置值'}
            }
        }
    }

    @staticmethod
    def execute(tool_name, params, user_id):
        """Execute a tool and return result"""
        if tool_name == 'device_list':
            return AgentTools._device_list(params, user_id)
        elif tool_name == 'device_control':
            return AgentTools._device_control(params, user_id)
        elif tool_name == 'device_query':
            return AgentTools._device_query(params, user_id)
        elif tool_name == 'data_query':
            return AgentTools._data_query(params, user_id)
        elif tool_name == 'config_get':
            return AgentTools._config_get(params, user_id)
        elif tool_name == 'config_set':
            return AgentTools._config_set(params, user_id)
        else:
            return {'error': f'Unknown tool: {tool_name}'}

    @staticmethod
    def _device_list(params, user_id):
        """List devices, optionally filter by name"""
        name = params.get('name', '')
        query = Device.query.filter_by(user_id=user_id)
        if name:
            query = query.filter(Device.name.ilike(f'%{name}%'))
        devices = query.all()
        return {
            'devices': [
                {'id': d.id, 'name': d.name, 'status': d.status, 'type': d.device_type}
                for d in devices
            ]
        }

    @staticmethod
    def _device_query(params, user_id):
        """Query device info"""
        device_id = params.get('device_id')
        device = Device.query.filter_by(id=device_id, user_id=user_id).first()
        if not device:
            return {'error': 'Device not found'}
        return {
            'name': device.name,
            'status': device.status,
            'type': device.device_type,
            'last_seen': device.last_seen.isoformat() if device.last_seen else None
        }

    @staticmethod
    def _data_query(params, user_id):
        """Query device data"""
        device_id = params.get('device_id')
        limit = params.get('limit', 10)
        device = Device.query.filter_by(id=device_id, user_id=user_id).first()
        if not device:
            return {'error': 'Device not found'}
        records = DataRecord.query.filter_by(device_id=device_id).order_by(
            DataRecord.timestamp.desc()
        ).limit(limit).all()
        return {'data': [r.to_dict() for r in records]}

    @staticmethod
    def _config_get(params, user_id):
        """Get device config"""
        device_id = params.get('device_id')
        device = Device.query.filter_by(id=device_id, user_id=user_id).first()
        if not device:
            return {'error': 'Device not found'}
        configs = DeviceConfig.query.filter_by(device_id=device_id).all()
        return {'config': {c.key: c.value for c in configs}}

    @staticmethod
    def _config_set(params, user_id):
        """Set device config"""
        device_id = params.get('device_id')
        key = params.get('key')
        value = params.get('value')
        device = Device.query.filter_by(id=device_id, user_id=user_id).first()
        if not device:
            return {'error': 'Device not found'}
        config = DeviceConfig.query.filter_by(device_id=device_id, key=key).first()
        if config:
            config.value = value
        else:
            config = DeviceConfig(device_id=device_id, key=key, value=value)
            db.session.add(config)
        db.session.commit()
        return {'success': True}

    @staticmethod
    def _device_control(params, user_id):
        """Control device (placeholder)"""
        device_id = params.get('device_id')
        command = params.get('command')
        device = Device.query.filter_by(id=device_id, user_id=user_id).first()
        if not device:
            return {'error': 'Device not found'}
        # TODO: Implement actual device control via MQTT/HTTP
        return {'success': True, 'message': f'Command {command} sent to device'}

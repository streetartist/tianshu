"""Initialize default data"""
import os
from app import create_app
from app.extensions import db
from app.models import DeviceTemplate, Agent, User


def init_db():
    """Create all database tables"""
    db.create_all()
    print("Database tables created.")


def init_default_user():
    """Create default admin user if not exists"""
    user = User.query.filter_by(username='admin').first()
    if not user:
        user = User(username='admin', email='admin@example.com')
        user.set_password('changeme')
        db.session.add(user)
        db.session.commit()
        print("Created default user: admin / admin123")
    else:
        print("Default user exists: admin")
    return user


def init_templates():
    """Initialize preset device templates"""
    templates = [
        {
            'name': 'desktop_dog',
            'display_name': '桌面电子狗',
            'description': '桌面智能伙伴，支持表情显示、语音交互',
            'icon': 'Monitor',
            'hardware_platform': 'esp32',
            'data_points': [
                {'name': 'temperature', 'type': 'float', 'unit': '°C', 'label': '温度'},
                {'name': 'humidity', 'type': 'float', 'unit': '%', 'label': '湿度'},
                {'name': 'light', 'type': 'int', 'unit': 'lux', 'label': '光照'},
                {'name': 'touch', 'type': 'bool', 'label': '触摸状态'}
            ],
            'commands': [
                {'name': 'set_expression', 'label': '设置表情',
                 'params': [{'name': 'expression', 'type': 'string', 'options': ['happy', 'sad', 'angry', 'sleep']}]},
                {'name': 'play_audio', 'label': '播放音频',
                 'params': [{'name': 'url', 'type': 'string'}]},
                {'name': 'set_led', 'label': '设置LED',
                 'params': [{'name': 'r', 'type': 'int', 'min': 0, 'max': 255},
                           {'name': 'g', 'type': 'int', 'min': 0, 'max': 255},
                           {'name': 'b', 'type': 'int', 'min': 0, 'max': 255}]}
            ],
            'config_schema': {
                'wifi_ssid': {'type': 'string', 'label': 'WiFi名称', 'required': True},
                'wifi_password': {'type': 'password', 'label': 'WiFi密码', 'required': True},
                'volume': {'type': 'number', 'label': '音量', 'min': 0, 'max': 100, 'default': 50},
                'brightness': {'type': 'number', 'label': '亮度', 'min': 0, 'max': 100, 'default': 80}
            },
            'comm_settings': {
                'heartbeat_interval': 60,
                'data_interval': 300,
                'offline_timeout': 180,
                'protocol': 'http'
            },
            'config_items': [
                {'key': 'volume', 'label': '音量', 'type': 'int', 'default_value': '50'},
                {'key': 'brightness', 'label': '亮度', 'type': 'int', 'default_value': '80'}
            ]
        },
        {
            'name': 'sensor_node',
            'display_name': '传感器节点',
            'description': '通用传感器采集节点，支持多种传感器',
            'icon': 'Odometer',
            'hardware_platform': 'esp32',
            'data_points': [
                {'name': 'temperature', 'type': 'float', 'unit': '°C', 'label': '温度'},
                {'name': 'humidity', 'type': 'float', 'unit': '%', 'label': '湿度'},
                {'name': 'pressure', 'type': 'float', 'unit': 'hPa', 'label': '气压'},
                {'name': 'battery', 'type': 'int', 'unit': '%', 'label': '电量'}
            ],
            'commands': [
                {'name': 'set_interval', 'label': '设置采集间隔',
                 'params': [{'name': 'seconds', 'type': 'int', 'min': 10, 'max': 3600}]},
                {'name': 'calibrate', 'label': '校准传感器', 'params': []}
            ],
            'config_schema': {
                'wifi_ssid': {'type': 'string', 'label': 'WiFi名称', 'required': True},
                'wifi_password': {'type': 'password', 'label': 'WiFi密码', 'required': True},
                'report_interval': {'type': 'number', 'label': '上报间隔(秒)', 'min': 10, 'default': 60}
            },
            'comm_settings': {
                'heartbeat_interval': 120,
                'data_interval': 60,
                'offline_timeout': 300,
                'protocol': 'http'
            },
            'config_items': [
                {'key': 'report_interval', 'label': '上报间隔(秒)', 'type': 'int', 'default_value': '60'}
            ]
        }
    ]

    for t in templates:
        existing = DeviceTemplate.query.filter_by(name=t['name']).first()
        if not existing:
            template = DeviceTemplate(**t)
            db.session.add(template)
            print(f"Created template: {t['name']}")
        else:
            # Update existing preset
            for key, value in t.items():
                setattr(existing, key, value)
            print(f"Updated template: {t['name']}")

    db.session.commit()


def init_agents():
    """Initialize preset AI agents"""
    # Get first user as owner (or skip if no users)
    user = User.query.first()
    if not user:
        print("No user found, skipping agent initialization")
        return

    agents = [
        {
            'name': '智能家居助手',
            'description': '专业的智能家居控制助手，可以帮助您管理和控制IoT设备',
            'icon': 'Service',
            'model_name': 'gpt-4',
            'system_prompt': '''你是天枢IoT平台的智能家居助手。你的职责是：
1. 帮助用户查询和控制智能设备
2. 分析设备数据并提供建议
3. 回答关于智能家居的问题

请用简洁友好的语气回复用户。当需要控制设备时，使用提供的工具函数。''',
            'persona': {
                'name': '小枢',
                'role': '智能家居助手',
                'style': 'friendly',
                'traits': '专业、耐心、乐于助人'
            },
            'tools_enabled': ['device_control', 'device_query', 'data_query', 'config_get'],
            'temperature': 0.7,
            'max_tokens': 2000,
            'context_window': 20,
            'output_format': 'text'
        },
        {
            'name': '数据分析师',
            'description': '专注于IoT数据分析，提供数据洞察和趋势预测',
            'icon': 'DataAnalysis',
            'model_name': 'gpt-4',
            'system_prompt': '''你是一个专业的IoT数据分析师。你的任务是：
1. 分析设备上报的传感器数据
2. 识别数据异常和趋势
3. 提供数据可视化建议
4. 给出优化建议

请用专业但易懂的方式解释数据分析结果。''',
            'persona': {
                'name': '数析',
                'role': '数据分析专家',
                'style': 'professional',
                'traits': '严谨、数据驱动、善于发现规律'
            },
            'tools_enabled': ['device_query', 'data_query'],
            'temperature': 0.3,
            'max_tokens': 3000,
            'context_window': 30,
            'output_format': 'markdown'
        }
    ]

    for a in agents:
        existing = Agent.query.filter_by(name=a['name'], user_id=user.id).first()
        if not existing:
            agent = Agent(user_id=user.id, **a)
            db.session.add(agent)
            print(f"Created agent: {a['name']}")
        else:
            print(f"Agent exists: {a['name']}")

    db.session.commit()


if __name__ == '__main__':
    app = create_app()
    with app.app_context():
        # 创建数据库表
        init_db()
        # 创建默认用户
        user = init_default_user()
        # 初始化模板
        init_templates()
        # 初始化AI代理
        init_agents()
        print("Initialization complete.")

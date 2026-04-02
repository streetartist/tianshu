"""API Configuration Model"""
from datetime import datetime
from ..extensions import db


class ApiConfig(db.Model):
    """Third-party API Configuration"""
    __tablename__ = 'api_configs'

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    description = db.Column(db.String(500), default='')  # 简介
    api_type = db.Column(db.String(50), nullable=False)  # ai_model, normal_api, python_handler

    # Endpoint - 完整URL，不再拼接
    base_url = db.Column(db.String(500), default='')  # 完整API地址
    api_key = db.Column(db.String(500))  # AI模型需要

    # Request configuration (普通API使用)
    request_method = db.Column(db.String(10), default='GET')  # GET, POST, PUT, DELETE
    request_headers = db.Column(db.JSON, default=dict)  # 额外请求头
    request_body_template = db.Column(db.Text, default='')  # 请求体模板

    # Response configuration (普通API使用)
    response_extract = db.Column(db.String(500), default='')  # JSONPath提取
    process_code = db.Column(db.Text, default='')  # Python代码处理

    # Python handler code (python_handler mode)
    handler_code = db.Column(db.Text, default='')  # 完整Python处理器代码

    # Settings
    is_active = db.Column(db.Boolean, default=True)
    proxy_mode = db.Column(db.String(20), default='server')  # server, direct
    timeout = db.Column(db.Integer, default=30)  # seconds

    # Statistics
    total_calls = db.Column(db.Integer, default=0)
    failed_calls = db.Column(db.Integer, default=0)

    # Ownership
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'))

    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    def to_dict(self, include_key=False):
        data = {
            'id': self.id,
            'name': self.name,
            'description': self.description or '',
            'api_type': self.api_type,
            'base_url': self.base_url or '',
            'request_method': self.request_method,
            'request_headers': self.request_headers or {},
            'request_body_template': self.request_body_template or '',
            'response_extract': self.response_extract or '',
            'process_code': self.process_code or '',
            'handler_code': self.handler_code or '',
            'is_active': self.is_active,
            'proxy_mode': self.proxy_mode,
            'timeout': self.timeout,
            'total_calls': self.total_calls,
            'created_at': self.created_at.isoformat()
        }
        if include_key:
            data['api_key'] = self.api_key
        return data

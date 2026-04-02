"""API Gateway"""
import re
import json
import threading
import traceback
from flask import request, jsonify
from flask_jwt_extended import jwt_required, get_jwt_identity
import requests
from . import api_v1
from ...extensions import db
from ...models import ApiConfig, Device


def render_template(template, params):
    """渲染模板，替换 {{param}} 变量"""
    if not template:
        return template
    result = template
    for key, value in params.items():
        result = result.replace('{{' + key + '}}', json.dumps(value) if isinstance(value, (dict, list)) else str(value))
    return result


def extract_response(data, path):
    """从响应中提取数据，支持简单的JSONPath如 $.choices[0].message.content"""
    if not path or not path.startswith('$.'):
        return data
    try:
        parts = path[2:].replace('[', '.').replace(']', '').split('.')
        result = data
        for part in parts:
            if part.isdigit():
                result = result[int(part)]
            elif part:
                result = result[part]
        return result
    except:
        return data


def execute_process_code(code, params, response_data):
    """执行Python处理代码（沙箱环境）"""
    if not code:
        return response_data
    try:
        local_vars = {'params': params, 'response': response_data, 'result': response_data}
        exec(code, {'__builtins__': {'str': str, 'int': int, 'float': float, 'list': list, 'dict': dict, 'len': len, 'json': json}}, local_vars)
        return local_vars.get('result', response_data)
    except Exception as e:
        return {'error': f'Process code error: {str(e)}', 'original': response_data}


def execute_handler_code(code, params, timeout=30):
    """执行完整Python处理器代码（支持所有库）"""
    if not code:
        return {'error': 'No handler code provided'}

    result = {'value': None, 'error': None}

    def run_code():
        try:
            # 创建执行环境，允许导入所有库
            global_vars = {
                '__builtins__': __builtins__,
                'params': params,
                'result': None
            }
            exec(code, global_vars)
            result['value'] = global_vars.get('result')
        except Exception as e:
            result['error'] = f'{type(e).__name__}: {str(e)}\n{traceback.format_exc()}'

    # 在线程中执行，支持超时
    thread = threading.Thread(target=run_code)
    thread.start()
    thread.join(timeout=timeout)

    if thread.is_alive():
        return {'error': f'Handler execution timeout ({timeout}s)'}

    if result['error']:
        return {'error': result['error']}

    return result['value']


@api_v1.route('/gateway/configs', methods=['GET'])
@jwt_required()
def list_api_configs():
    """List API configurations"""
    user_id = get_jwt_identity()
    configs = ApiConfig.query.filter_by(user_id=user_id).all()

    return jsonify({
        'code': 0,
        'data': [c.to_dict() for c in configs]
    })


@api_v1.route('/gateway/configs', methods=['POST'])
@jwt_required()
def create_api_config():
    """Create API configuration"""
    user_id = get_jwt_identity()
    data = request.get_json()

    # python_handler类型不需要base_url
    if data.get('api_type') == 'python_handler':
        if not data.get('name') or not data.get('handler_code'):
            return jsonify({'code': 1001, 'message': 'Missing required fields'}), 400
    else:
        if not data or not data.get('name') or not data.get('base_url'):
            return jsonify({'code': 1001, 'message': 'Missing required fields'}), 400

    config = ApiConfig(
        name=data['name'],
        description=data.get('description', ''),
        api_type=data.get('api_type', 'normal_api'),
        base_url=data.get('base_url', ''),
        api_key=data.get('api_key'),
        proxy_mode=data.get('proxy_mode', 'server'),
        request_method=data.get('request_method', 'GET'),
        request_headers=data.get('request_headers'),
        request_body_template=data.get('request_body_template', ''),
        response_extract=data.get('response_extract', ''),
        process_code=data.get('process_code', ''),
        handler_code=data.get('handler_code', ''),
        timeout=data.get('timeout', 30),
        user_id=user_id
    )

    db.session.add(config)
    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'API config created',
        'data': config.to_dict()
    })


@api_v1.route('/gateway/configs/<int:config_id>', methods=['GET'])
@jwt_required()
def get_api_config(config_id):
    """Get API config detail"""
    user_id = get_jwt_identity()
    config = ApiConfig.query.filter_by(id=config_id, user_id=user_id).first()

    if not config:
        return jsonify({'code': 3001, 'message': 'Config not found'}), 404

    return jsonify({
        'code': 0,
        'data': config.to_dict(include_key=True)
    })


@api_v1.route('/gateway/configs/<int:config_id>', methods=['PUT'])
@jwt_required()
def update_api_config(config_id):
    """Update API configuration"""
    user_id = get_jwt_identity()
    config = ApiConfig.query.filter_by(id=config_id, user_id=user_id).first()

    if not config:
        return jsonify({'code': 3001, 'message': 'Config not found'}), 404

    data = request.get_json()
    fields = ['name', 'description', 'api_type', 'base_url', 'api_key', 'proxy_mode',
              'request_method', 'request_headers', 'request_body_template',
              'response_extract', 'process_code', 'handler_code', 'timeout', 'is_active']
    for f in fields:
        if f in data:
            setattr(config, f, data[f])

    db.session.commit()
    return jsonify({'code': 0, 'data': config.to_dict()})


@api_v1.route('/gateway/configs/<int:config_id>', methods=['DELETE'])
@jwt_required()
def delete_api_config(config_id):
    """Delete API configuration"""
    user_id = get_jwt_identity()
    config = ApiConfig.query.filter_by(id=config_id, user_id=user_id).first()

    if not config:
        return jsonify({'code': 3001, 'message': 'Config not found'}), 404

    db.session.delete(config)
    db.session.commit()
    return jsonify({'code': 0, 'message': 'Config deleted'})


@api_v1.route('/gateway/proxy/<int:config_id>', methods=['POST'])
@jwt_required()
def proxy_request(config_id):
    """Proxy API request"""
    user_id = get_jwt_identity()
    config = ApiConfig.query.filter_by(id=config_id, user_id=user_id).first()

    if not config:
        return jsonify({'code': 3001, 'message': 'Config not found'}), 404

    data = request.get_json()
    endpoint = data.get('endpoint', '')
    method = data.get('method', 'GET')
    payload = data.get('data')

    url = f"{config.base_url.rstrip('/')}/{endpoint.lstrip('/')}"
    headers = {'Content-Type': 'application/json'}

    if config.api_key:
        headers['Authorization'] = f'Bearer {config.api_key}'

    try:
        resp = requests.request(
            method=method,
            url=url,
            json=payload,
            headers=headers,
            timeout=config.timeout
        )
        config.total_calls += 1
        db.session.commit()

        return jsonify({
            'code': 0,
            'data': resp.json()
        })
    except Exception as e:
        config.failed_calls += 1
        db.session.commit()
        return jsonify({'code': 3001, 'message': str(e)}), 500


@api_v1.route('/gateway/device/configs', methods=['GET'])
def device_get_api_configs():
    """设备获取可用的API配置列表"""
    device_id = request.args.get('device_id')
    secret_key = request.args.get('secret_key') or request.headers.get('X-Secret-Key')

    if not device_id or not secret_key:
        return jsonify({'code': 1001, 'message': 'Missing credentials'}), 400

    device = Device.query.filter_by(device_id=device_id, secret_key=secret_key).first()
    if not device:
        return jsonify({'code': 2001, 'message': 'Invalid credentials'}), 401

    # 获取设备所属用户的API配置
    configs = ApiConfig.query.filter_by(user_id=device.user_id, is_active=True).all()

    result = []
    for c in configs:
        item = {
            'id': c.id,
            'name': c.name,
            'api_type': c.api_type,
            'proxy_mode': c.proxy_mode,
            'timeout': c.timeout
        }
        # Python处理器返回参数模板
        if c.api_type == 'python_handler':
            item['request_body_template'] = c.request_body_template
        # 直连模式返回完整配置
        elif c.proxy_mode == 'direct':
            item['base_url'] = c.base_url
            item['api_key'] = c.api_key
            item['request_method'] = c.request_method
            item['request_headers'] = c.request_headers
            item['request_body_template'] = c.request_body_template
            item['response_extract'] = c.response_extract
        result.append(item)

    return jsonify({'code': 0, 'data': result})


@api_v1.route('/gateway/device/call/<int:config_id>', methods=['POST'])
def device_call_api(config_id):
    """设备调用API（服务器转发模式）"""
    data = request.get_json()
    device_id = data.get('device_id')
    secret_key = data.get('secret_key')
    params = data.get('params', {})

    if not device_id or not secret_key:
        return jsonify({'code': 1001, 'message': 'Missing credentials'}), 400

    device = Device.query.filter_by(device_id=device_id, secret_key=secret_key).first()
    if not device:
        return jsonify({'code': 2001, 'message': 'Invalid credentials'}), 401

    config = ApiConfig.query.filter_by(id=config_id, user_id=device.user_id, is_active=True).first()
    if not config:
        return jsonify({'code': 3001, 'message': 'API config not found'}), 404

    # Python处理器模式：直接执行代码
    if config.api_type == 'python_handler':
        try:
            config.total_calls += 1
            response_data = execute_handler_code(
                config.handler_code,
                params,
                timeout=config.timeout or 30
            )
            db.session.commit()

            if isinstance(response_data, dict) and 'error' in response_data and len(response_data) == 1:
                config.failed_calls += 1
                db.session.commit()
                return jsonify({'code': 3003, 'message': response_data['error']}), 500

            return jsonify({'code': 0, 'data': response_data})
        except Exception as e:
            config.failed_calls += 1
            db.session.commit()
            return jsonify({'code': 3002, 'message': str(e)}), 500

    # 构建请求 - 直接使用完整URL
    url = config.base_url

    headers = {'Content-Type': 'application/json'}
    headers.update(config.request_headers or {})
    if config.api_key:
        headers['Authorization'] = f'Bearer {config.api_key}'

    # 渲染请求体模板
    body = None
    if config.request_body_template:
        try:
            body_str = render_template(config.request_body_template, params)
            body = json.loads(body_str)
        except:
            body = params
    else:
        body = params

    try:
        resp = requests.request(
            method=config.request_method or 'POST',
            url=url,
            json=body,
            headers=headers,
            timeout=config.timeout or 30
        )
        config.total_calls += 1

        response_data = resp.json()

        # 提取响应数据
        if config.response_extract:
            response_data = extract_response(response_data, config.response_extract)

        # 执行处理代码
        if config.process_code:
            response_data = execute_process_code(config.process_code, params, response_data)

        db.session.commit()
        return jsonify({'code': 0, 'data': response_data})

    except Exception as e:
        config.failed_calls += 1
        db.session.commit()
        return jsonify({'code': 3002, 'message': str(e)}), 500

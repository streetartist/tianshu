"""Device Template API"""
import re
from flask import request, jsonify
from flask_jwt_extended import jwt_required, get_jwt_identity
from . import api_v1
from ...extensions import db
from ...models import DeviceTemplate


@api_v1.route('/templates', methods=['GET'])
@jwt_required()
def list_templates():
    """List all device templates"""
    templates = DeviceTemplate.query.filter_by(is_active=True).all()
    return jsonify({
        'code': 0,
        'data': [t.to_dict() for t in templates]
    })


@api_v1.route('/templates/<int:template_id>', methods=['GET'])
@jwt_required()
def get_template(template_id):
    """Get template detail"""
    template = DeviceTemplate.query.filter_by(
        id=template_id, is_active=True
    ).first()

    if not template:
        return jsonify({'code': 2001, 'message': 'Template not found'}), 404

    return jsonify({
        'code': 0,
        'data': template.to_dict()
    })


@api_v1.route('/templates', methods=['POST'])
@jwt_required()
def create_template():
    """Create device template"""
    user_id = get_jwt_identity()
    data = request.get_json()

    if not data or not data.get('name'):
        return jsonify({'code': 1001, 'message': 'Missing name'}), 400

    name = data['name']
    if not re.match(r'^[a-z][a-z0-9_]*$', name):
        return jsonify({'code': 1002, 'message': 'Name must be lowercase with underscores'}), 400

    existing = DeviceTemplate.query.filter_by(name=name).first()
    if existing:
        return jsonify({'code': 1003, 'message': 'Template name already exists'}), 400

    template = DeviceTemplate(
        name=name,
        display_name=data.get('display_name', name),
        description=data.get('description'),
        icon=data.get('icon', 'Cpu'),
        user_id=user_id,
        data_points=data.get('data_points', []),
        commands=data.get('commands', []),
        config_schema=data.get('config_schema', {}),
        config_items=data.get('config_items', []),
        comm_settings=data.get('comm_settings', {
            'heartbeat_interval': 60,
            'data_interval': 300,
            'offline_timeout': 180,
            'protocol': 'http'
        }),
        hardware_platform=data.get('hardware_platform', 'esp32')
    )

    db.session.add(template)
    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'Template created',
        'data': template.to_dict()
    })


@api_v1.route('/templates/<int:template_id>', methods=['PUT'])
@jwt_required()
def update_template(template_id):
    """Update template"""
    template = DeviceTemplate.query.filter_by(
        id=template_id, is_active=True
    ).first()

    if not template:
        return jsonify({'code': 2001, 'message': 'Template not found'}), 404

    data = request.get_json()

    if data.get('display_name'):
        template.display_name = data['display_name']
    if data.get('description') is not None:
        template.description = data['description']
    if data.get('icon'):
        template.icon = data['icon']
    if data.get('data_points') is not None:
        template.data_points = data['data_points']
    if data.get('commands') is not None:
        template.commands = data['commands']
    if data.get('config_schema') is not None:
        template.config_schema = data['config_schema']
    if data.get('config_items') is not None:
        template.config_items = data['config_items']
    if data.get('comm_settings') is not None:
        template.comm_settings = data['comm_settings']
    if data.get('hardware_platform'):
        template.hardware_platform = data['hardware_platform']

    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'Template updated',
        'data': template.to_dict()
    })


@api_v1.route('/templates/<int:template_id>', methods=['DELETE'])
@jwt_required()
def delete_template(template_id):
    """Delete template"""
    template = DeviceTemplate.query.filter_by(id=template_id).first()

    if not template:
        return jsonify({'code': 2001, 'message': 'Template not found'}), 404

    db.session.delete(template)
    db.session.commit()

    return jsonify({'code': 0, 'message': 'Template deleted'})

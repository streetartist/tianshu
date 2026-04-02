"""Operation Log API"""
from flask import request, jsonify
from flask_jwt_extended import jwt_required, get_jwt_identity
from . import api_v1
from ...models import OperationLog


@api_v1.route('/logs', methods=['GET'])
@jwt_required()
def list_logs():
    """List operation logs"""
    page = request.args.get('page', 1, type=int)
    per_page = request.args.get('per_page', 20, type=int)

    query = OperationLog.query.order_by(OperationLog.created_at.desc())
    pagination = query.paginate(page=page, per_page=per_page)

    return jsonify({
        'code': 0,
        'data': {
            'items': [log.to_dict() for log in pagination.items],
            'total': pagination.total,
            'page': page,
            'pages': pagination.pages
        }
    })

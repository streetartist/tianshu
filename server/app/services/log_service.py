"""Log Service"""
from flask import request
from ..models import OperationLog
from ..extensions import db


class LogService:
    """Operation log service"""

    @staticmethod
    def log(user_id, action, resource_type=None, resource_id=None, detail=None):
        log = OperationLog(
            user_id=user_id,
            action=action,
            resource_type=resource_type,
            resource_id=resource_id,
            detail=detail,
            ip_address=request.remote_addr if request else None
        )
        db.session.add(log)
        db.session.commit()
        return log

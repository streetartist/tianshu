"""Data Service"""
from datetime import datetime
from ..models import DataRecord
from ..extensions import db


class DataService:
    """Device data service"""

    @staticmethod
    def save_record(device_id, data_type, data):
        record = DataRecord(
            device_id=device_id,
            data_type=data_type,
            data=data
        )
        db.session.add(record)
        db.session.commit()
        return record

    @staticmethod
    def get_records(device_id, limit=100, data_type=None):
        query = DataRecord.query.filter_by(device_id=device_id)
        if data_type:
            query = query.filter_by(data_type=data_type)
        return query.order_by(DataRecord.timestamp.desc()).limit(limit).all()

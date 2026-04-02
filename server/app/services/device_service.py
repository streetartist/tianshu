"""Device Service"""
from datetime import datetime, timedelta
from ..models import Device, DeviceGroup
from ..extensions import db


class DeviceService:
    """Device management service"""

    OFFLINE_TIMEOUT = 5  # minutes

    @staticmethod
    def get_device_by_id(device_id):
        return Device.query.filter_by(device_id=device_id).first()

    @staticmethod
    def update_device_status(device, status='online'):
        device.status = status
        device.last_seen = datetime.utcnow()
        db.session.commit()

    @staticmethod
    def get_offline_devices(user_id, timeout_minutes=5):
        threshold = datetime.utcnow() - timedelta(minutes=timeout_minutes)
        return Device.query.filter(
            Device.user_id == user_id,
            Device.last_seen < threshold
        ).all()

    @staticmethod
    def check_all_device_status():
        """Check and update offline devices"""
        threshold = datetime.utcnow() - timedelta(minutes=DeviceService.OFFLINE_TIMEOUT)
        Device.query.filter(
            Device.status == 'online',
            Device.last_seen < threshold
        ).update({'status': 'offline'})
        db.session.commit()

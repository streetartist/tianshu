"""Background Scheduler for TianShu"""
from apscheduler.schedulers.background import BackgroundScheduler
from apscheduler.triggers.interval import IntervalTrigger

scheduler = BackgroundScheduler()


def check_device_status(app):
    """Periodic task to check device online status"""
    from app.services.device_service import DeviceService

    with app.app_context():
        DeviceService.check_all_device_status()


def init_scheduler(app):
    """Initialize scheduler with Flask app"""
    if not scheduler.running:
        # Check device status every minute
        scheduler.add_job(
            func=check_device_status,
            trigger=IntervalTrigger(minutes=1),
            id='check_device_status',
            name='Check device online status',
            replace_existing=True,
            args=[app]
        )
        scheduler.start()

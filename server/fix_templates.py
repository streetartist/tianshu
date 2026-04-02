"""Fix templates - set all is_active to True"""
from app import create_app
from app.extensions import db
from app.models import DeviceTemplate


def fix_templates():
    templates = DeviceTemplate.query.all()
    for t in templates:
        if not t.is_active:
            t.is_active = True
            print(f"Fixed: {t.name}")
    db.session.commit()

    # Show all templates
    print("\nAll templates:")
    for t in DeviceTemplate.query.all():
        print(f"  - {t.name}: is_active={t.is_active}")


if __name__ == '__main__':
    app = create_app()
    with app.app_context():
        fix_templates()

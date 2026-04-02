"""API Gateway Service"""
import requests
from ..models import ApiConfig
from ..extensions import db


class ApiGatewayService:
    """API proxy and management service"""

    @staticmethod
    def proxy_request(config_id, endpoint, method='GET', data=None):
        config = ApiConfig.query.get(config_id)
        if not config or not config.is_active:
            return {'error': 'API config not found or inactive'}

        url = f"{config.base_url.rstrip('/')}/{endpoint.lstrip('/')}"
        headers = {}

        if config.api_key:
            headers['Authorization'] = f'Bearer {config.api_key}'

        try:
            resp = requests.request(
                method=method,
                url=url,
                json=data,
                headers=headers,
                timeout=config.timeout
            )
            config.total_calls += 1
            db.session.commit()
            return resp.json()
        except Exception as e:
            config.failed_calls += 1
            db.session.commit()
            return {'error': str(e)}

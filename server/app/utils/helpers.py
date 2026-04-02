"""Helper Functions"""


def success_response(data=None, message='success'):
    return {
        'code': 0,
        'message': message,
        'data': data
    }


def error_response(code, message):
    return {
        'code': code,
        'message': message,
        'data': None
    }

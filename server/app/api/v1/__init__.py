"""API v1 Blueprint"""
from flask import Blueprint

api_v1 = Blueprint('api_v1', __name__)

from . import auth, devices, data, gateway, agents, config, templates, logs

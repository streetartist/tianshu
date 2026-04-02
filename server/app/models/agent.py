"""AI Agent Model"""
from datetime import datetime
from ..extensions import db


class Agent(db.Model):
    """AI Agent Configuration"""
    __tablename__ = 'agents'

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    description = db.Column(db.Text)
    icon = db.Column(db.String(50), default='Robot')

    # AI Model Configuration
    api_config_id = db.Column(db.Integer, db.ForeignKey('api_configs.id'))
    model_name = db.Column(db.String(50))
    temperature = db.Column(db.Float, default=0.7)
    max_tokens = db.Column(db.Integer, default=2000)
    top_p = db.Column(db.Float, default=1.0)
    frequency_penalty = db.Column(db.Float, default=0.0)
    presence_penalty = db.Column(db.Float, default=0.0)

    # Prompt System
    system_prompt = db.Column(db.Text)
    prompt_template = db.Column(db.Text)  # User message template with variables

    # Persona preset
    persona = db.Column(db.JSON, default=dict)  # {name, role, style, traits}

    # Tools & Capabilities
    tools_enabled = db.Column(db.JSON, default=list)  # ['device_control', 'data_query', 'api_call']
    custom_tools = db.Column(db.JSON, default=list)  # Custom tool definitions

    # Context & Memory
    context_window = db.Column(db.Integer, default=20)
    memory_enabled = db.Column(db.Boolean, default=False)
    memory_summary = db.Column(db.Text)  # Long-term memory summary

    # Triggers
    triggers = db.Column(db.JSON, default=list)  # [{type, config}]

    # Knowledge Base
    knowledge_docs = db.Column(db.JSON, default=list)  # Document references

    # Output Settings
    output_format = db.Column(db.String(20), default='text')  # text, json, markdown

    is_active = db.Column(db.Boolean, default=True)
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'))
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    # Relationships
    api_config = db.relationship('ApiConfig', backref='agents')

    def to_dict(self):
        return {
            'id': self.id,
            'name': self.name,
            'description': self.description,
            'icon': self.icon,
            'api_config_id': self.api_config_id,
            'model_name': self.model_name,
            'temperature': self.temperature,
            'max_tokens': self.max_tokens,
            'top_p': self.top_p,
            'system_prompt': self.system_prompt,
            'prompt_template': self.prompt_template,
            'persona': self.persona or {},
            'tools_enabled': self.tools_enabled or [],
            'custom_tools': self.custom_tools or [],
            'context_window': self.context_window,
            'memory_enabled': self.memory_enabled,
            'triggers': self.triggers or [],
            'output_format': self.output_format,
            'is_active': self.is_active,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }

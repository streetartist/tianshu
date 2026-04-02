"""Agent Conversation Model"""
from datetime import datetime
from ..extensions import db


class AgentConversation(db.Model):
    """Agent conversation session"""
    __tablename__ = 'agent_conversations'

    id = db.Column(db.Integer, primary_key=True)
    agent_id = db.Column(db.Integer, db.ForeignKey('agents.id'), nullable=False)
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'), nullable=False)
    title = db.Column(db.String(200))
    is_active = db.Column(db.Boolean, default=True)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    messages = db.relationship('AgentMessage', backref='conversation', lazy='dynamic')

    def to_dict(self):
        return {
            'id': self.id,
            'agent_id': self.agent_id,
            'title': self.title,
            'is_active': self.is_active,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'message_count': self.messages.count()
        }


class AgentMessage(db.Model):
    """Agent conversation message"""
    __tablename__ = 'agent_messages'

    id = db.Column(db.Integer, primary_key=True)
    conversation_id = db.Column(db.Integer, db.ForeignKey('agent_conversations.id'), nullable=False)
    role = db.Column(db.String(20), nullable=False)  # user, assistant, system, tool
    content = db.Column(db.Text)
    tool_calls = db.Column(db.JSON)  # Tool call results
    created_at = db.Column(db.DateTime, default=datetime.utcnow)

    def to_dict(self):
        return {
            'id': self.id,
            'role': self.role,
            'content': self.content,
            'tool_calls': self.tool_calls,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }

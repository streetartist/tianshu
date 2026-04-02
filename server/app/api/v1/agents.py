"""AI Agent API"""
import json
from flask import request, jsonify, Response, stream_with_context
from flask_jwt_extended import jwt_required, get_jwt_identity, verify_jwt_in_request
from . import api_v1
from ...extensions import db
from ...models import Agent, ApiConfig, Device, DeviceAgent, AgentConversation, AgentMessage
from ...services.agent_tools import AgentTools
from ...services.agent_service import AgentService


@api_v1.route('/agents', methods=['GET'])
@jwt_required()
def list_agents():
    """List AI agents"""
    user_id = get_jwt_identity()
    agents = Agent.query.filter_by(user_id=user_id).all()

    return jsonify({
        'code': 0,
        'data': [a.to_dict() for a in agents]
    })


@api_v1.route('/agents', methods=['POST'])
@jwt_required()
def create_agent():
    """Create AI agent"""
    user_id = get_jwt_identity()
    data = request.get_json()

    if not data or not data.get('name'):
        return jsonify({'code': 1001, 'message': 'Missing name'}), 400

    agent = Agent(
        name=data['name'],
        description=data.get('description'),
        icon=data.get('icon', 'Robot'),
        api_config_id=data.get('api_config_id'),
        model_name=data.get('model_name'),
        system_prompt=data.get('system_prompt'),
        prompt_template=data.get('prompt_template'),
        persona=data.get('persona'),
        temperature=data.get('temperature', 0.7),
        max_tokens=data.get('max_tokens', 2000),
        top_p=data.get('top_p', 1.0),
        tools_enabled=data.get('tools_enabled', []),
        custom_tools=data.get('custom_tools', []),
        context_window=data.get('context_window', 20),
        memory_enabled=data.get('memory_enabled', False),
        output_format=data.get('output_format', 'text'),
        user_id=int(user_id)
    )

    db.session.add(agent)
    db.session.commit()

    return jsonify({
        'code': 0,
        'message': 'Agent created',
        'data': agent.to_dict()
    })


@api_v1.route('/agents/<int:agent_id>', methods=['GET'])
@jwt_required()
def get_agent(agent_id):
    """Get agent detail"""
    user_id = get_jwt_identity()
    agent = Agent.query.filter_by(id=agent_id, user_id=int(user_id)).first()
    if not agent:
        return jsonify({'code': 3001, 'message': 'Agent not found'}), 404
    return jsonify({'code': 0, 'data': agent.to_dict()})


@api_v1.route('/agents/<int:agent_id>', methods=['PUT'])
@jwt_required()
def update_agent(agent_id):
    """Update agent"""
    user_id = get_jwt_identity()
    agent = Agent.query.filter_by(id=agent_id, user_id=int(user_id)).first()
    if not agent:
        return jsonify({'code': 3001, 'message': 'Agent not found'}), 404

    data = request.get_json()
    fields = ['name', 'description', 'icon', 'api_config_id', 'model_name',
              'system_prompt', 'prompt_template', 'persona', 'temperature',
              'max_tokens', 'top_p', 'tools_enabled', 'custom_tools',
              'context_window', 'memory_enabled', 'output_format']
    for f in fields:
        if f in data:
            setattr(agent, f, data[f])
    db.session.commit()
    return jsonify({'code': 0, 'data': agent.to_dict()})


@api_v1.route('/agents/<int:agent_id>/bind', methods=['POST'])
@jwt_required()
def bind_device(agent_id):
    """Bind agent to device"""
    user_id = get_jwt_identity()
    data = request.get_json()

    agent = Agent.query.filter_by(id=agent_id, user_id=user_id).first()
    if not agent:
        return jsonify({'code': 3001, 'message': 'Agent not found'}), 404

    device = Device.query.filter_by(
        id=data.get('device_id'), user_id=user_id
    ).first()
    if not device:
        return jsonify({'code': 2001, 'message': 'Device not found'}), 404

    binding = DeviceAgent(device_id=device.id, agent_id=agent.id)
    db.session.add(binding)
    db.session.commit()

    return jsonify({'code': 0, 'message': 'Bound successfully'})


@api_v1.route('/agents/<int:agent_id>/chat', methods=['POST'])
@jwt_required()
def chat_with_agent(agent_id):
    """Chat with AI agent (with tool calling support)"""
    import traceback
    try:
        user_id = int(get_jwt_identity())
        agent = Agent.query.filter_by(id=agent_id, user_id=user_id).first()

        if not agent:
            return jsonify({'code': 3001, 'message': 'Agent not found'}), 404

        if not agent.api_config_id:
            return jsonify({'code': 3001, 'message': 'Agent has no API config'}), 400

        data = request.get_json(silent=True)
        if data is None or not isinstance(data, dict):
            return jsonify({'code': 1001, 'message': 'Invalid or missing JSON body'}), 400

        message = data.get('message', '')
        conversation_id = data.get('conversation_id')

        # Get or create conversation
        if conversation_id:
            conv = AgentConversation.query.filter_by(
                id=conversation_id, agent_id=agent_id, user_id=user_id
            ).first()
            if not conv:
                return jsonify({'code': 3001, 'message': 'Conversation not found'}), 404
        else:
            conv = AgentConversation(agent_id=agent_id, user_id=user_id, title=message[:50])
            db.session.add(conv)
            db.session.commit()

        # Save user message
        user_msg = AgentMessage(conversation_id=conv.id, role='user', content=message)
        db.session.add(user_msg)
        db.session.commit()

        # Build context from history
        context = []
        history = AgentMessage.query.filter_by(conversation_id=conv.id).order_by(
            AgentMessage.created_at.desc()
        ).limit(agent.context_window).all()
        history.reverse()
        for msg in history[:-1]:  # Exclude the message we just added
            context.append({'role': msg.role, 'content': msg.content})

        # Call AgentService with tool support
        result = AgentService.chat(agent_id, message, context=context, user_id=user_id)

        if 'error' in result:
            print(f"[Agent Chat Error] AgentService error: {result['error']}")
            debug = result.get('debug', [])
            for log in debug:
                print(f"[Debug] {log}")
            return jsonify({'code': 3001, 'message': result['error'], 'debug': debug}), 500

        reply = result.get('content', '')
        tool_results = result.get('tool_results', [])
        debug = result.get('debug', [])

        # Print debug logs
        for log in debug:
            print(f"[Debug] {log}")

        # Save assistant message
        assistant_msg = AgentMessage(conversation_id=conv.id, role='assistant', content=reply)
        db.session.add(assistant_msg)
        db.session.commit()

        return jsonify({
            'code': 0,
            'data': {
                'reply': reply,
                'conversation_id': conv.id,
                'tool_results': tool_results,
                'debug': debug
            }
        })
    except Exception as e:
        print(f"[Agent Chat Error] {str(e)}")
        print(traceback.format_exc())
        return jsonify({'code': 5000, 'message': f'Internal error: {str(e)}'}), 500


@api_v1.route('/agents/<int:agent_id>/chat/stream', methods=['POST'])
def chat_with_agent_stream(agent_id):
    """Stream chat with AI agent (SSE)"""
    import traceback

    # 手动验证JWT
    try:
        verify_jwt_in_request()
        user_id = int(get_jwt_identity())
    except Exception as e:
        return jsonify({'code': 401, 'message': 'Unauthorized'}), 401

    agent = Agent.query.filter_by(id=agent_id, user_id=user_id).first()
    if not agent:
        return jsonify({'code': 3001, 'message': 'Agent not found'}), 404

    if not agent.api_config_id:
        return jsonify({'code': 3001, 'message': 'Agent has no API config'}), 400

    data = request.get_json(silent=True)
    if data is None or not isinstance(data, dict):
        return jsonify({'code': 1001, 'message': 'Invalid JSON'}), 400

    message = data.get('message', '')
    conversation_id = data.get('conversation_id')

    # Get or create conversation
    if conversation_id:
        conv = AgentConversation.query.filter_by(
            id=conversation_id, agent_id=agent_id, user_id=user_id
        ).first()
        if not conv:
            return jsonify({'code': 3001, 'message': 'Conversation not found'}), 404
    else:
        conv = AgentConversation(agent_id=agent_id, user_id=user_id, title=message[:50])
        db.session.add(conv)
        db.session.commit()

    # Save user message
    user_msg = AgentMessage(conversation_id=conv.id, role='user', content=message)
    db.session.add(user_msg)
    db.session.commit()

    # Build context
    context = []
    history = AgentMessage.query.filter_by(conversation_id=conv.id).order_by(
        AgentMessage.created_at.desc()
    ).limit(agent.context_window).all()
    history.reverse()
    for msg in history[:-1]:
        context.append({'role': msg.role, 'content': msg.content})

    conv_id = conv.id

    def generate():
        try:
            for event in AgentService.chat_stream(agent_id, message, context=context, user_id=user_id):
                yield f"data: {json.dumps(event, ensure_ascii=False)}\n\n"

            # 获取最终结果并保存
            final_content = event.get('content', '') if event.get('type') == 'done' else ''
            if final_content:
                assistant_msg = AgentMessage(conversation_id=conv_id, role='assistant', content=final_content)
                db.session.add(assistant_msg)
                db.session.commit()
        except Exception as e:
            yield f"data: {json.dumps({'type': 'error', 'message': str(e)}, ensure_ascii=False)}\n\n"

    return Response(
        stream_with_context(generate()),
        mimetype='text/event-stream',
        headers={'Cache-Control': 'no-cache', 'X-Accel-Buffering': 'no'}
    )


@api_v1.route('/agents/<int:agent_id>', methods=['DELETE'])
@jwt_required()
def delete_agent(agent_id):
    """Delete agent"""
    user_id = get_jwt_identity()
    agent = Agent.query.filter_by(id=agent_id, user_id=int(user_id)).first()
    if not agent:
        return jsonify({'code': 3001, 'message': 'Agent not found'}), 404

    # Delete related conversations and messages
    conversations = AgentConversation.query.filter_by(agent_id=agent_id).all()
    for conv in conversations:
        AgentMessage.query.filter_by(conversation_id=conv.id).delete()
    AgentConversation.query.filter_by(agent_id=agent_id).delete()
    DeviceAgent.query.filter_by(agent_id=agent_id).delete()

    db.session.delete(agent)
    db.session.commit()
    return jsonify({'code': 0, 'message': 'Agent deleted'})


@api_v1.route('/agents/<int:agent_id>/conversations', methods=['GET'])
@jwt_required()
def list_conversations(agent_id):
    """List agent conversations"""
    user_id = int(get_jwt_identity())
    agent = Agent.query.filter_by(id=agent_id, user_id=user_id).first()
    if not agent:
        return jsonify({'code': 3001, 'message': 'Agent not found'}), 404

    conversations = AgentConversation.query.filter_by(
        agent_id=agent_id, user_id=user_id
    ).order_by(AgentConversation.updated_at.desc()).all()

    return jsonify({
        'code': 0,
        'data': [c.to_dict() for c in conversations]
    })


@api_v1.route('/agents/<int:agent_id>/conversations/<int:conv_id>', methods=['GET'])
@jwt_required()
def get_conversation(agent_id, conv_id):
    """Get conversation with messages"""
    user_id = int(get_jwt_identity())
    conv = AgentConversation.query.filter_by(
        id=conv_id, agent_id=agent_id, user_id=user_id
    ).first()
    if not conv:
        return jsonify({'code': 3001, 'message': 'Conversation not found'}), 404

    messages = AgentMessage.query.filter_by(conversation_id=conv_id).order_by(
        AgentMessage.created_at.asc()
    ).all()

    return jsonify({
        'code': 0,
        'data': {
            **conv.to_dict(),
            'messages': [m.to_dict() for m in messages]
        }
    })


@api_v1.route('/agents/<int:agent_id>/conversations/<int:conv_id>', methods=['DELETE'])
@jwt_required()
def delete_conversation(agent_id, conv_id):
    """Delete conversation"""
    user_id = int(get_jwt_identity())
    conv = AgentConversation.query.filter_by(
        id=conv_id, agent_id=agent_id, user_id=user_id
    ).first()
    if not conv:
        return jsonify({'code': 3001, 'message': 'Conversation not found'}), 404

    AgentMessage.query.filter_by(conversation_id=conv_id).delete()
    db.session.delete(conv)
    db.session.commit()
    return jsonify({'code': 0, 'message': 'Conversation deleted'})


@api_v1.route('/agents/tools', methods=['GET'])
@jwt_required()
def list_tools():
    """List available tools"""
    return jsonify({
        'code': 0,
        'data': list(AgentTools.AVAILABLE_TOOLS.values())
    })

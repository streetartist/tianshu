"""Agent Service"""
import json
import traceback
import requests
from ..models import Agent, ApiConfig
from .agent_tools import AgentTools


def convert_tools_to_openai_format(tools_enabled):
    """将启用的工具转换为OpenAI格式"""
    tools = []
    for tool_name in tools_enabled:
        if tool_name in AgentTools.AVAILABLE_TOOLS:
            tool_def = AgentTools.AVAILABLE_TOOLS[tool_name]
            # 转换为OpenAI function格式
            properties = {}
            required = []
            for param_name, param_info in tool_def.get('parameters', {}).items():
                properties[param_name] = {
                    'type': param_info.get('type', 'string'),
                    'description': param_info.get('description', '')
                }
                if 'default' not in param_info:
                    required.append(param_name)

            tools.append({
                'type': 'function',
                'function': {
                    'name': tool_def['name'],
                    'description': tool_def['description'],
                    'parameters': {
                        'type': 'object',
                        'properties': properties,
                        'required': required
                    }
                }
            })
    return tools


class AgentService:
    """AI Agent service with tool calling support"""

    @staticmethod
    def chat(agent_id, message, context=None, user_id=None):
        """与Agent对话，支持工具调用"""
        debug_logs = []
        try:
            agent = Agent.query.get(agent_id)
            if not agent or not agent.is_active:
                return {'error': 'Agent not found'}

            api_config = ApiConfig.query.get(agent.api_config_id)
            if not api_config:
                return {'error': 'API config not found'}

            # 构建消息列表
            messages = []
            if agent.system_prompt:
                messages.append({'role': 'system', 'content': agent.system_prompt})

            if context:
                messages.extend(context[-agent.context_window:])

            messages.append({'role': 'user', 'content': message})

            # 获取启用的工具
            tools = None
            if agent.tools_enabled:
                tools = convert_tools_to_openai_format(agent.tools_enabled)
                debug_logs.append(f"Tools enabled: {agent.tools_enabled}")

            # 调用AI模型
            debug_logs.append(f"=== Round 1 ===")
            response = AgentService._call_model(api_config, agent, messages, tools)
            debug_logs.append(f"AI Response: {json.dumps(response, ensure_ascii=False)[:500]}")

            if 'error' in response:
                return {'error': response['error'], 'debug': debug_logs}

            if 'choices' not in response or not response['choices']:
                return {'error': f'Invalid API response', 'debug': debug_logs}

            # 处理工具调用循环
            max_iterations = 5
            iteration = 0
            all_tool_results = []

            while iteration < max_iterations:
                assistant_message = response.get('choices', [{}])[0].get('message', {})
                debug_logs.append(f"Assistant message: {json.dumps(assistant_message, ensure_ascii=False)[:300]}")

                tool_calls = assistant_message.get('tool_calls')
                if not tool_calls:
                    return {
                        'content': assistant_message.get('content', ''),
                        'tool_results': all_tool_results,
                        'debug': debug_logs
                    }

                debug_logs.append(f"Tool calls: {len(tool_calls)}")
                messages.append(assistant_message)

                for tool_call in tool_calls:
                    func = tool_call.get('function', {})
                    tool_name = func.get('name')
                    try:
                        tool_args = json.loads(func.get('arguments', '{}'))
                    except:
                        tool_args = {}

                    debug_logs.append(f"Calling tool: {tool_name}, args: {tool_args}")
                    result = AgentTools.execute(tool_name, tool_args, user_id)
                    debug_logs.append(f"Tool result: {json.dumps(result, ensure_ascii=False)[:200]}")

                    all_tool_results.append({
                        'tool': tool_name,
                        'args': tool_args,
                        'result': result
                    })

                    messages.append({
                        'role': 'tool',
                        'tool_call_id': tool_call.get('id'),
                        'content': json.dumps(result, ensure_ascii=False)
                    })

                iteration += 1
                debug_logs.append(f"=== Round {iteration + 1} ===")
                response = AgentService._call_model(api_config, agent, messages, tools)
                debug_logs.append(f"AI Response: {json.dumps(response, ensure_ascii=False)[:500]}")

                if 'error' in response:
                    return {'error': response['error'], 'debug': debug_logs}

                if 'choices' not in response or not response['choices']:
                    return {'error': 'Invalid API response after tool call', 'debug': debug_logs}

            return {'error': 'Max tool call iterations exceeded', 'debug': debug_logs}
        except Exception as e:
            debug_logs.append(f"Exception: {str(e)}")
            return {'error': f'Service error: {str(e)}', 'debug': debug_logs}

    @staticmethod
    def chat_stream(agent_id, message, context=None, user_id=None):
        """流式对话，使用生成器返回事件"""
        try:
            agent = Agent.query.get(agent_id)
            if not agent or not agent.is_active:
                yield {'type': 'error', 'message': 'Agent not found'}
                return

            api_config = ApiConfig.query.get(agent.api_config_id)
            if not api_config:
                yield {'type': 'error', 'message': 'API config not found'}
                return

            # 构建消息列表
            messages = []
            if agent.system_prompt:
                messages.append({'role': 'system', 'content': agent.system_prompt})
            if context:
                messages.extend(context[-agent.context_window:])
            messages.append({'role': 'user', 'content': message})

            # 获取工具
            tools = None
            if agent.tools_enabled:
                tools = convert_tools_to_openai_format(agent.tools_enabled)

            max_iterations = 5
            iteration = 0
            all_tool_results = []

            while iteration < max_iterations:
                iteration += 1
                yield {'type': 'round', 'round': iteration}

                # 流式调用模型
                content_buffer = ''
                tool_calls_buffer = []

                for chunk in AgentService._call_model_stream(api_config, agent, messages, tools):
                    if chunk.get('type') == 'content':
                        content_buffer += chunk.get('text', '')
                        yield {'type': 'content', 'text': chunk.get('text', '')}
                    elif chunk.get('type') == 'tool_call':
                        tool_calls_buffer = chunk.get('tool_calls', [])
                    elif chunk.get('type') == 'error':
                        yield chunk
                        return

                # 检查是否有工具调用
                if not tool_calls_buffer:
                    yield {'type': 'done', 'content': content_buffer, 'tool_results': all_tool_results}
                    return

                # 处理工具调用
                assistant_message = {
                    'role': 'assistant',
                    'content': content_buffer or None,
                    'tool_calls': tool_calls_buffer
                }
                messages.append(assistant_message)

                for tool_call in tool_calls_buffer:
                    func = tool_call.get('function', {})
                    tool_name = func.get('name')
                    try:
                        tool_args = json.loads(func.get('arguments', '{}'))
                    except:
                        tool_args = {}

                    yield {'type': 'tool_call', 'tool': tool_name, 'args': tool_args}

                    result = AgentTools.execute(tool_name, tool_args, user_id)
                    all_tool_results.append({
                        'tool': tool_name,
                        'args': tool_args,
                        'result': result
                    })

                    yield {'type': 'tool_result', 'tool': tool_name, 'result': result}

                    messages.append({
                        'role': 'tool',
                        'tool_call_id': tool_call.get('id'),
                        'content': json.dumps(result, ensure_ascii=False)
                    })

            yield {'type': 'error', 'message': 'Max iterations exceeded'}
        except Exception as e:
            yield {'type': 'error', 'message': str(e)}

    @staticmethod
    def _call_model_stream(api_config, agent, messages, tools=None):
        """流式调用AI模型"""
        url = api_config.base_url

        headers = {
            'Content-Type': 'application/json',
            'Authorization': f'Bearer {api_config.api_key}'
        }

        data = {
            'model': agent.model_name,
            'messages': messages,
            'temperature': agent.temperature,
            'max_tokens': agent.max_tokens,
            'stream': True
        }

        if tools:
            data['tools'] = tools

        try:
            resp = requests.post(
                url,
                json=data,
                headers=headers,
                timeout=api_config.timeout or 120,
                stream=True
            )

            if resp.status_code >= 400:
                yield {'type': 'error', 'message': f'HTTP {resp.status_code}'}
                return

            tool_calls_data = {}

            for line in resp.iter_lines():
                if not line:
                    continue
                line = line.decode('utf-8')
                if not line.startswith('data: '):
                    continue
                data_str = line[6:]
                if data_str == '[DONE]':
                    break

                try:
                    chunk = json.loads(data_str)
                    delta = chunk.get('choices', [{}])[0].get('delta', {})

                    # 处理内容
                    if delta.get('content'):
                        yield {'type': 'content', 'text': delta['content']}

                    # 处理工具调用
                    if delta.get('tool_calls'):
                        for tc in delta['tool_calls']:
                            idx = tc.get('index', 0)
                            if idx not in tool_calls_data:
                                tool_calls_data[idx] = {
                                    'id': tc.get('id', ''),
                                    'type': 'function',
                                    'function': {'name': '', 'arguments': ''}
                                }
                            if tc.get('id'):
                                tool_calls_data[idx]['id'] = tc['id']
                            if tc.get('function', {}).get('name'):
                                tool_calls_data[idx]['function']['name'] = tc['function']['name']
                            if tc.get('function', {}).get('arguments'):
                                tool_calls_data[idx]['function']['arguments'] += tc['function']['arguments']
                except:
                    continue

            # 返回完整的工具调用
            if tool_calls_data:
                yield {'type': 'tool_call', 'tool_calls': list(tool_calls_data.values())}

        except Exception as e:
            yield {'type': 'error', 'message': str(e)}

    @staticmethod
    def _call_model(api_config, agent, messages, tools=None):
        """调用AI模型"""
        url = api_config.base_url

        print(f"[AgentService] Calling API: {url}")
        print(f"[AgentService] Model: {agent.model_name}")

        headers = {
            'Content-Type': 'application/json',
            'Authorization': f'Bearer {api_config.api_key}'
        }

        data = {
            'model': agent.model_name,
            'messages': messages,
            'temperature': agent.temperature,
            'max_tokens': agent.max_tokens
        }

        if tools:
            data['tools'] = tools

        try:
            resp = requests.post(
                url,
                json=data,
                headers=headers,
                timeout=api_config.timeout or 60
            )
        except Exception as e:
            return {'error': f'Request error: {str(e)}'}

        try:
            payload = resp.json()
        except ValueError:
            text = (resp.text or '').strip()
            snippet = text[:500]
            return {'error': f'Non-JSON response (HTTP {resp.status_code}): {snippet}'}

        if resp.status_code >= 400:
            return {'error': f'HTTP {resp.status_code}: {payload}'}

        return payload

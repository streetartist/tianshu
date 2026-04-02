import 'api_service.dart';

/// AI服务
class AiService {
  final ApiService _api;

  AiService(this._api);

  /// 获取Agent列表
  Future<List<dynamic>> getAgents() async {
    final response = await _api.get('/api/v1/agents');
    return response.data['agents'] ?? [];
  }

  /// 发送对话消息
  Future<Map<String, dynamic>> chat(
      String agentId, String message, String? conversationId) async {
    final response = await _api.post(
      '/api/v1/agents/$agentId/chat',
      data: {
        'message': message,
        if (conversationId != null) 'conversation_id': conversationId,
      },
    );
    return response.data;
  }
}

import 'api_service.dart';

/// 设备服务
class DeviceService {
  final ApiService _api;

  DeviceService(this._api);

  /// 获取设备列表
  Future<List<dynamic>> getDevices() async {
    final response = await _api.get('/api/v1/devices');
    return response.data['devices'] ?? [];
  }

  /// 获取设备详情
  Future<Map<String, dynamic>> getDevice(String id) async {
    final response = await _api.get('/api/v1/devices/$id');
    return response.data;
  }

  /// 获取设备配置
  Future<Map<String, dynamic>> getDeviceConfig(String id) async {
    final response = await _api.get('/api/v1/devices/$id/config');
    return response.data;
  }

  /// 更新设备配置
  Future<void> updateDeviceConfig(
      String id, Map<String, dynamic> config) async {
    await _api.put('/api/v1/devices/$id/config', data: config);
  }
}

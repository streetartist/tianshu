import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:socket_io_client/socket_io_client.dart' as IO;

/// 天枢IoT服务
class IoTService extends ChangeNotifier {
  static final IoTService _instance = IoTService._();
  static IoTService get instance => _instance;
  IoTService._() {
    _initDefaultBindings();
  }

  String _serverUrl = '';
  IO.Socket? _socket;
  bool _connected = false;
  final _deviceData = <String, Map<String, dynamic>>{};
  final _deviceStatus = <String, String>{};
  final _slotBindings = <String, String>{}; // slotId -> deviceId
  final _deviceSecrets = <String, String>{}; // deviceId -> secretKey
  final _deviceHistory = <String, Map<String, List<double>>>{}; // deviceId -> {field -> values}
  static const int _maxHistoryLength = 50;

  void _initDefaultBindings() {
    bindDevice('default', 'bf308c98-83ca-4022-8c79-9e1b6bae10a9', 'a0e7009d210c40a59781fa8b77f70c1653d3e638f39e4d30a87acd3f6343f656');
  }

  /// 初始化并连接WebSocket
  Future<void> init({required String serverUrl}) async {
    _serverUrl = serverUrl.replaceAll(RegExp(r'/$'), '');
    _connectWebSocket();
    // 获取初始数据
    for (final deviceId in _slotBindings.values.toSet()) {
      await _fetchDeviceData(deviceId);
      await _fetchDeviceStatus(deviceId);
    }
  }

  /// 建立WebSocket连接
  void _connectWebSocket() {
    _socket?.dispose();
    _socket = IO.io(_serverUrl, <String, dynamic>{
      'transports': ['websocket'],
      'autoConnect': true,
    });

    _socket!.onConnect((_) {
      print('[IoT] WebSocket connected');
      _connected = true;
      notifyListeners();
    });

    _socket!.onDisconnect((_) {
      print('[IoT] WebSocket disconnected');
      _connected = false;
      notifyListeners();
    });

    // 接收命令发送结果
    _socket!.on('command_result', (data) {
      print('[IoT] Command result: $data');
    });

    // 接收设备数据更新
    _socket!.on('device_data', (data) {
      if (data is Map && data['device_id'] != null) {
        final deviceId = data['device_id'].toString();
        final newData = Map<String, dynamic>.from(data['data'] ?? {});
        _deviceData[deviceId] = newData;
        _appendHistory(deviceId, newData);
        notifyListeners();
      }
    });

    // 接收设备状态更新
    _socket!.on('device_status', (data) {
      if (data is Map && data['device_id'] != null) {
        final deviceId = data['device_id'].toString();
        _deviceStatus[deviceId] = data['status']?.toString() ?? 'offline';
        notifyListeners();
      }
    });

    _socket!.connect();
  }

  void bindDevice(String slotId, String deviceId, [String secretKey = '']) {
    _slotBindings[slotId] = deviceId;
    if (secretKey.isNotEmpty) {
      _deviceSecrets[deviceId] = secretKey;
    }
    _fetchDeviceData(deviceId);
  }

  String? _getDeviceId(String slotId) {
    if (slotId == 'default') return _slotBindings.values.firstOrNull;
    return _slotBindings[slotId];
  }

  /// 获取数值
  double getValue(String slotId, String field) {
    final deviceId = _getDeviceId(slotId);
    if (deviceId == null) return 0;
    final data = _deviceData[deviceId];
    if (data == null) return 0;
    final value = data[field];
    if (value is num) return value.toDouble();
    return double.tryParse(value?.toString() ?? '') ?? 0;
  }

  /// 获取字符串
  String getString(String slotId, String field) {
    final deviceId = _getDeviceId(slotId);
    if (deviceId == null) return '';
    return _deviceData[deviceId]?[field]?.toString() ?? '';
  }

  /// 获取布尔值
  bool getBool(String slotId, String field) {
    // 特殊处理：在线状态
    if (field == 'status' || field == '_online') {
      return isOnline(slotId);
    }
    final deviceId = _getDeviceId(slotId);
    if (deviceId == null) return false;
    final value = _deviceData[deviceId]?[field];
    if (value is bool) return value;
    if (value is num) return value != 0;
    if (value is String) return value == 'true' || value == 'on' || value == '1';
    return false;
  }

  /// 判断设备是否在线
  bool isOnline(String slotId) {
    final deviceId = _getDeviceId(slotId);
    if (deviceId == null) return false;
    return _deviceStatus[deviceId] == 'online';
  }

  /// 获取历史数据
  List<double> getHistory(String slotId, String field) {
    final deviceId = _getDeviceId(slotId);
    if (deviceId == null) return [];
    final history = _deviceHistory[deviceId]?[field];
    if (history != null && history.isNotEmpty) return List.unmodifiable(history);
    final current = getValue(slotId, field);
    return [current];
  }

  /// 获取统计数据
  Map<String, double> getStats(String slotId, String field) {
    final deviceId = _getDeviceId(slotId);
    if (deviceId == null) return {};
    final stats = _deviceData[deviceId]?['${field}_stats'];
    if (stats is Map) return stats.map((k, v) => MapEntry(k.toString(), (v as num).toDouble()));
    return {};
  }

  /// 发送命令
  /// [command] 命令名称
  /// [params] 命令参数，可以是Map、基本类型或null
  Future<bool> send(String slotId, String command, [dynamic params]) async {
    final deviceId = _getDeviceId(slotId);
    if (deviceId == null) return false;
    final secretKey = _deviceSecrets[deviceId] ?? '';

    if (!_connected || _socket == null) {
      print('[IoT] WebSocket未连接，无法发送命令');
      return false;
    }

    // 构建命令参数
    Map<String, dynamic>? cmdParams;
    if (params != null) {
      if (params is Map) {
        cmdParams = Map<String, dynamic>.from(params);
      } else {
        cmdParams = {'value': params};
      }
    }

    _socket!.emit('send_command', {
      'device_id': deviceId,
      'secret_key': secretKey,
      'command': command,
      'params': cmdParams ?? {},
    });
    return true;
  }

  /// 将一条数据追加到历史缓冲区
  void _appendHistory(String deviceId, Map<String, dynamic> data) {
    final historyMap = _deviceHistory.putIfAbsent(deviceId, () => {});
    for (final entry in data.entries) {
      final v = entry.value;
      double? numVal;
      if (v is num) {
        numVal = v.toDouble();
      } else if (v is String) {
        numVal = double.tryParse(v);
      }
      if (numVal != null) {
        final list = historyMap.putIfAbsent(entry.key, () => []);
        list.add(numVal);
        if (list.length > _maxHistoryLength) {
          list.removeAt(0);
        }
      }
    }
  }

  Future<void> _fetchDeviceData(String deviceId) async {
    if (_serverUrl.isEmpty) return;
    final secretKey = _deviceSecrets[deviceId] ?? '';
    try {
      final resp = await http.get(
        Uri.parse('$_serverUrl/api/v1/devices/$deviceId/data?secret_key=$secretKey&data_type=sensor&limit=$_maxHistoryLength'),
        headers: {'Content-Type': 'application/json'},
      );
      if (resp.statusCode == 200) {
        final json = jsonDecode(resp.body);
        if (json['code'] == 0 && json['data'] is List && json['data'].isNotEmpty) {
          final records = json['data'] as List;
          // 最新一条作为当前数据
          _deviceData[deviceId] = Map<String, dynamic>.from(records[0]['data'] ?? {});
          // 从旧到新构建历史
          _deviceHistory[deviceId] = {};
          for (var i = records.length - 1; i >= 0; i--) {
            final recordData = records[i]['data'];
            if (recordData is Map) {
              _appendHistory(deviceId, Map<String, dynamic>.from(recordData));
            }
          }
          notifyListeners();
        }
      }
    } catch (e) {
      print('获取数据失败: $e');
    }
  }

  Future<void> _fetchDeviceStatus(String deviceId) async {
    if (_serverUrl.isEmpty) return;
    final secretKey = _deviceSecrets[deviceId] ?? '';
    try {
      final resp = await http.get(
        Uri.parse('$_serverUrl/api/v1/devices/$deviceId/info?secret_key=$secretKey'),
        headers: {'Content-Type': 'application/json'},
      );
      if (resp.statusCode == 200) {
        final json = jsonDecode(resp.body);
        if (json['code'] == 0 && json['data'] != null) {
          _deviceStatus[deviceId] = json['data']['status'] ?? 'offline';
          notifyListeners();
        }
      }
    } catch (e) {
      _deviceStatus[deviceId] = 'offline';
    }
  }

  /// WebSocket是否已连接
  bool get isConnected => _connected;

  void dispose() {
    _socket?.dispose();
    _socket = null;
  }
}

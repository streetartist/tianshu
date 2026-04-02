import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'services/iot_service.dart';
import 'app.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // 从本地存储加载配置
  final prefs = await SharedPreferences.getInstance();
  final serverUrl = prefs.getString('server_url') ?? 'https://tianshuapi.streetartist.top';

  // 初始化IoT服务
  if (serverUrl.isNotEmpty) {
    await IoTService.instance.init(serverUrl: serverUrl);
  }

  runApp(const MyApp());
}

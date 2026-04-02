import '../models/generated_project.dart';
import '../../../data/models/app_setting.dart';

/// main.dart生成器
class MainGenerator {
  static GeneratedFile generate({List<AppSetting>? settings}) {
    final appSettings = settings ?? [];

    // 获取server_url的默认值
    final serverUrlSetting = appSettings.firstWhere(
      (s) => s.key == 'server_url',
      orElse: () => AppSetting(
        key: 'server_url',
        label: 'IoT平台地址',
        defaultValue: 'https://tianshuapi.streetartist.top',
      ),
    );
    final defaultServerUrl = serverUrlSetting.defaultValue;

    return GeneratedFile(
      path: 'lib/main.dart',
      content: '''import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'services/iot_service.dart';
import 'app.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // 从本地存储加载配置
  final prefs = await SharedPreferences.getInstance();
  final serverUrl = prefs.getString('server_url') ?? '$defaultServerUrl';

  // 初始化IoT服务
  if (serverUrl.isNotEmpty) {
    await IoTService.instance.init(serverUrl: serverUrl);
  }

  runApp(const MyApp());
}
''',
    );
  }
}

import '../models/generated_project.dart';
import '../../../data/models/app_setting.dart';

/// 设置页面生成器
class SettingsPageGenerator {
  static GeneratedFile generate(List<AppSetting> settings) {
    // 筛选出可见的设置项
    final visibleSettings = settings.where((s) => s.visible).toList();

    // 生成控制器声明
    final controllers = settings.map((s) =>
      "  final _${s.key}Controller = TextEditingController();"
    ).join('\n');

    // 生成加载默认值代码
    final loadDefaults = settings.map((s) =>
      "    _${s.key}Controller.text = prefs.getString('${s.key}') ?? '${s.defaultValue}';"
    ).join('\n');

    // 生成保存代码
    final saveCode = settings.map((s) =>
      "    await prefs.setString('${s.key}', _${s.key}Controller.text.trim());"
    ).join('\n');

    // 生成dispose代码
    final disposeCode = settings.map((s) =>
      "    _${s.key}Controller.dispose();"
    ).join('\n');

    // 生成可见设置项的UI
    final fieldsUI = visibleSettings.map((s) => _buildFieldUI(s)).join('\n');

    return GeneratedFile(
      path: 'lib/pages/settings_page.dart',
      content: '''import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/iot_service.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
$controllers
  bool _loading = false;

  @override
  void initState() {
    super.initState();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    final prefs = await SharedPreferences.getInstance();
$loadDefaults
  }

  Future<void> _saveSettings() async {
    setState(() => _loading = true);
    final prefs = await SharedPreferences.getInstance();
$saveCode

    // 重新初始化IoT服务
    await IoTService.instance.init(
      serverUrl: _server_urlController.text.trim(),
    );

    setState(() => _loading = false);
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('设置已保存')),
      );
      Navigator.pop(context);
    }
  }

  @override
  void dispose() {
$disposeCode
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('设置')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
$fieldsUI
          const SizedBox(height: 24),
          ElevatedButton(
            onPressed: _loading ? null : _saveSettings,
            child: _loading
                ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2))
                : const Text('保存'),
          ),
        ],
      ),
    );
  }
}
''',
    );
  }

  static String _buildFieldUI(AppSetting setting) {
    final obscure = setting.inputType == 'password' ? 'obscureText: true,' : '';
    return '''          TextField(
            controller: _${setting.key}Controller,
            decoration: const InputDecoration(
              labelText: '${setting.label}',
              border: OutlineInputBorder(),
            ),
            $obscure
          ),
          const SizedBox(height: 16),''';
  }
}
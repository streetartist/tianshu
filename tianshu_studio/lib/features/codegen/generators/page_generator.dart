import '../../../data/models/models.dart';
import '../../../data/models/layout_models.dart';
import '../../../data/models/app_setting.dart';
import '../models/generated_project.dart';
import 'widget_code_generator.dart';

/// 页面生成器
class PageGenerator {
  static List<GeneratedFile> generate(Project project) {
    final files = <GeneratedFile>[];

    // 生成首页
    files.add(_generateHomePage(project));

    // 为每个画布页面生成对应页面文件
    for (final page in project.pages) {
      files.add(_generatePage(page));
    }

    return files;
  }

  /// 从PageLayout生成页面（新布局系统）
  static GeneratedFile generateFromLayout(
    PageLayout layout,
    String className, {
    List<AppSetting>? settings,
    bool enableDeviceManagement = false,
  }) {
    final widgetCode = WidgetCodeGenerator.generateFromLayout(layout.children);
    final hasVisibleSettings = settings?.any((s) => s.visible) ?? false;

    return GeneratedFile(
      path: 'lib/pages/${_toSnakeCase(layout.name)}_page.dart',
      content: _buildPageCode(className, layout.name, widgetCode, hasVisibleSettings, enableDeviceManagement),
    );
  }

  static String _buildPageCode(String className, String title, String widgets, bool hasVisibleSettings, bool enableDeviceManagement) {
    final imports = <String>[];
    if (hasVisibleSettings) imports.add("import 'settings_page.dart';");
    if (enableDeviceManagement) imports.add("import 'device_management_page.dart';");
    final importsCode = imports.join('\n');

    final actions = <String>[];
    if (enableDeviceManagement) {
      actions.add('''
          IconButton(
            icon: const Icon(Icons.devices),
            onPressed: () => Navigator.push(
              context,
              MaterialPageRoute(builder: (_) => const DeviceManagementPage()),
            ),
            tooltip: '设备管理',
          ),''');
    }
    if (hasVisibleSettings) {
      actions.add('''
          IconButton(
            icon: const Icon(Icons.settings),
            onPressed: () => Navigator.push(
              context,
              MaterialPageRoute(builder: (_) => const SettingsPage()),
            ),
          ),''');
    }

    final actionsCode = actions.isNotEmpty
        ? '''
        actions: [${actions.join('')}
        ],'''
        : '';

    return '''import 'package:flutter/material.dart';
import '../widgets/widgets.dart';
import '../services/iot_service.dart';
$importsCode

class $className extends StatefulWidget {
  const $className({super.key});

  @override
  State<$className> createState() => _${className}State();
}

class _${className}State extends State<$className> {
  final iot = IoTService.instance;

  @override
  void initState() {
    super.initState();
    iot.addListener(_onDataChanged);
  }

  @override
  void dispose() {
    iot.removeListener(_onDataChanged);
    super.dispose();
  }

  void _onDataChanged() => setState(() {});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('$title'),$actionsCode
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
$widgets
        ],
      ),
    );
  }
}
''';
  }

  static GeneratedFile _generateHomePage(Project project) {
    return GeneratedFile(
      path: 'lib/pages/home_page.dart',
      content: '''import 'package:flutter/material.dart';

class HomePage extends StatelessWidget {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('首页')),
      body: const Center(child: Text('欢迎使用')),
    );
  }
}
''',
    );
  }

  static GeneratedFile _generatePage(CanvasPage page) {
    final widgetCode = WidgetCodeGenerator.generateWidgets(page.widgets);

    return GeneratedFile(
      path: 'lib/pages/${_toSnakeCase(page.name)}_page.dart',
      content: '''import 'package:flutter/material.dart';

class ${_toPascalCase(page.name)}Page extends StatelessWidget {
  const ${_toPascalCase(page.name)}Page({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('${page.name}')),
      body: Stack(
        children: [
$widgetCode
        ],
      ),
    );
  }
}
''',
    );
  }

  static String _toSnakeCase(String s) {
    if (RegExp(r'[\u4e00-\u9fa5]').hasMatch(s)) {
      return 'main';
    }
    return s.replaceAllMapped(
      RegExp(r'[A-Z]'),
      (m) => '_${m.group(0)!.toLowerCase()}',
    ).replaceAll(' ', '_').toLowerCase();
  }

  static String _toPascalCase(String s) {
    if (RegExp(r'[\u4e00-\u9fa5]').hasMatch(s)) {
      return 'Main';
    }
    return s.split(RegExp(r'[_\s]+')).map((w) =>
      w.isEmpty ? '' : w[0].toUpperCase() + w.substring(1)
    ).join();
  }
}

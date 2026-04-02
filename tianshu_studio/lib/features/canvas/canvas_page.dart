import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../data/services/project_storage_service.dart';
import '../../shared/providers/app_providers.dart';
import '../codegen/generators/page_generator.dart';
import '../codegen/generators/service_generator.dart';
import '../codegen/generators/widgets_generator.dart';
import '../codegen/generators/pubspec_generator.dart';
import '../codegen/generators/main_generator.dart';
import '../codegen/generators/settings_page_generator.dart';
import '../codegen/generators/readme_generator.dart';
import '../codegen/generators/device_management_page_generator.dart';
import '../codegen/models/generated_project.dart';
import '../codegen/services/project_exporter.dart';
import 'state/layout_canvas_notifier.dart';
import 'widgets/widget_palette.dart';
import 'widgets/layout_canvas_area.dart';
import 'widgets/property_edit_panel.dart';
import 'widgets/slot_manager_dialog.dart';
import 'widgets/app_settings_dialog.dart';
import 'widgets/preview_dialog.dart';

/// 画布编辑器页面
class CanvasEditorPage extends ConsumerWidget {
  final String projectId;

  const CanvasEditorPage({super.key, required this.projectId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final project = ref.watch(currentProjectProvider);

    return Scaffold(
      appBar: AppBar(
        title: Text(project?.name ?? '编辑器'),
        actions: [
          IconButton(
            icon: const Icon(Icons.devices),
            onPressed: () => _showSlotManager(context),
            tooltip: '设备槽位',
          ),
          IconButton(
            icon: const Icon(Icons.settings),
            onPressed: () => _showAppSettings(context),
            tooltip: '应用设置',
          ),
          IconButton(
            icon: const Icon(Icons.save),
            onPressed: () => _saveProject(context, ref),
            tooltip: '保存',
          ),
          IconButton(
            icon: const Icon(Icons.play_arrow),
            onPressed: () => _showPreview(context),
            tooltip: '预览',
          ),
          IconButton(
            icon: const Icon(Icons.code),
            onPressed: () => _exportProject(context, ref),
            tooltip: '导出项目',
          ),
        ],
      ),
      body: const Row(
        children: [
          WidgetPalette(),
          Expanded(child: LayoutCanvasArea()),
          PropertyEditPanel(),
        ],
      ),
    );
  }

  void _showSlotManager(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => const SlotManagerDialog(),
    );
  }

  void _showAppSettings(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => const AppSettingsDialog(),
    );
  }

  void _showPreview(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => const PreviewDialog(),
    );
  }

  Future<void> _exportProject(BuildContext context, WidgetRef ref) async {
    final project = ref.read(currentProjectProvider);
    if (project == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('请先创建项目')),
      );
      return;
    }

    // 从画布状态获取布局
    final canvasState = ref.read(layoutCanvasProvider);
    final layout = canvasState.page;

    // 生成文件列表
    final files = <GeneratedFile>[];

    // 获取应用设置配置
    final appSettings = canvasState.appSettings.settings;

    // 入口文件（传递设置配置）
    files.add(MainGenerator.generate(settings: appSettings));

    // App配置（使用画布页面作为首页）
    files.add(_generateAppWithLayout(project.name, layout.name));

    // 从画布布局生成页面
    final className = '${_toPascalCase(layout.name)}Page';
    final enableDeviceManagement = canvasState.deviceConfig.enableDeviceManagement;
    files.add(PageGenerator.generateFromLayout(
      layout,
      className,
      settings: appSettings,
      enableDeviceManagement: enableDeviceManagement,
    ));

    // 获取设备槽配置，构建默认绑定（包含设备ID和密钥）
    final deviceBindings = <String, Map<String, String>>{};
    for (final slot in canvasState.deviceConfig.slots) {
      if (slot.defaultDeviceId.isNotEmpty) {
        deviceBindings[slot.id] = {
          'deviceId': slot.defaultDeviceId,
          'secretKey': slot.defaultSecretKey,
        };
      }
    }

    // 服务层（传递设备绑定）
    files.addAll(ServiceGenerator.generate(project, deviceBindings: deviceBindings));

    // 设置页面（只有有可见设置项时才生成）
    final hasVisibleSettings = appSettings.any((s) => s.visible);
    if (hasVisibleSettings) {
      files.add(SettingsPageGenerator.generate(appSettings));
    }

    // 设备管理页面（启用时才生成）
    if (enableDeviceManagement) {
      files.add(DeviceManagementPageGenerator.generate(canvasState.deviceConfig.slots));
    }

    // 控件库
    files.add(WidgetsGenerator.generate());

    // pubspec.yaml
    files.add(PubspecGenerator.generate(project));

    // README
    files.add(ReadmeGenerator.generate(project.name));

    final generatedProject = GeneratedProject(name: project.name, files: files);
    final success = await ProjectExporter.export(generatedProject);

    if (context.mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(success ? '项目导出成功' : '项目导出失败')),
      );
    }
  }

  GeneratedFile _generateAppWithLayout(String projectName, String pageName) {
    final className = '${_toPascalCase(pageName)}Page';
    final fileName = _toSnakeCase(pageName);
    return GeneratedFile(
      path: 'lib/app.dart',
      content: '''import 'package:flutter/material.dart';
import 'pages/${fileName}_page.dart';

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: '$projectName',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: const $className(),
      debugShowCheckedModeBanner: false,
    );
  }
}
''',
    );
  }

  String _toPascalCase(String s) {
    // 如果包含中文，使用默认名称
    if (RegExp(r'[\u4e00-\u9fa5]').hasMatch(s)) {
      return 'Main';
    }
    return s.split(RegExp(r'[_\s]+')).map((w) =>
      w.isEmpty ? '' : w[0].toUpperCase() + w.substring(1)
    ).join();
  }

  String _toSnakeCase(String s) {
    // 如果包含中文，使用默认名称
    if (RegExp(r'[\u4e00-\u9fa5]').hasMatch(s)) {
      return 'main';
    }
    return s.replaceAllMapped(
      RegExp(r'[A-Z]'),
      (m) => '_${m.group(0)!.toLowerCase()}',
    ).replaceAll(' ', '_').toLowerCase();
  }

  Future<void> _saveProject(BuildContext context, WidgetRef ref) async {
    final project = ref.read(currentProjectProvider);
    if (project == null) return;

    final success = await ProjectStorageService.saveProject(project);
    if (context.mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(success ? '项目已保存' : '保存失败')),
      );
    }
  }
}

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../data/models/models.dart';
import '../state/canvas_notifier.dart';
import '../../codegen/models/generated_project.dart';

/// 属性面板 - 右侧属性/代码面板
class PropertyPanel extends ConsumerWidget {
  const PropertyPanel({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final canvasState = ref.watch(canvasProvider);
    final selectedWidget = canvasState.widgets
        .where((w) => w.id == canvasState.selectedWidgetId)
        .firstOrNull;

    return Container(
      width: 280,
      decoration: BoxDecoration(
        border: Border(
          left: BorderSide(color: Theme.of(context).dividerColor),
        ),
      ),
      child: DefaultTabController(
        length: 2,
        child: Column(
          children: [
            const TabBar(tabs: [Tab(text: '属性'), Tab(text: '代码')]),
            Expanded(
              child: TabBarView(
                children: [
                  _PropertyTab(widget: selectedWidget),
                  const _CodeTab(),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _PropertyTab extends ConsumerWidget {
  final WidgetInstance? widget;
  const _PropertyTab({this.widget});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    if (widget == null) {
      return const Center(child: Text('选择控件查看属性'));
    }
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Text('类型: ${widget!.type}', style: const TextStyle(fontWeight: FontWeight.bold)),
        const SizedBox(height: 16),
        _buildPositionFields(ref),
        const Divider(),
        ..._buildPropertyFields(ref),
      ],
    );
  }

  Widget _buildPositionFields(WidgetRef ref) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text('位置', style: TextStyle(fontWeight: FontWeight.bold)),
        Row(
          children: [
            Expanded(child: _buildNumberField('X', widget!.x)),
            const SizedBox(width: 8),
            Expanded(child: _buildNumberField('Y', widget!.y)),
          ],
        ),
      ],
    );
  }

  Widget _buildNumberField(String label, double value) {
    return TextField(
      decoration: InputDecoration(labelText: label, isDense: true),
      controller: TextEditingController(text: value.toStringAsFixed(0)),
      keyboardType: TextInputType.number,
    );
  }

  List<Widget> _buildPropertyFields(WidgetRef ref) {
    return widget!.properties.entries.map((e) {
      return Padding(
        padding: const EdgeInsets.only(bottom: 8),
        child: _buildPropertyField(e.key, e.value, ref),
      );
    }).toList();
  }

  Widget _buildPropertyField(String key, dynamic value, WidgetRef ref) {
    if (value is bool) {
      return SwitchListTile(
        title: Text(key),
        value: value,
        onChanged: (v) => ref.read(canvasProvider.notifier)
            .updateWidgetProperty(widget!.id, key, v),
      );
    }
    return TextField(
      decoration: InputDecoration(labelText: key, isDense: true),
      controller: TextEditingController(text: value.toString()),
      onSubmitted: (v) => ref.read(canvasProvider.notifier)
          .updateWidgetProperty(widget!.id, key, v),
    );
  }
}

/// 代码预览标签页 - 支持多文件
class _CodeTab extends StatefulWidget {
  const _CodeTab();

  @override
  State<_CodeTab> createState() => _CodeTabState();
}

class _CodeTabState extends State<_CodeTab> {
  String? _selectedFile;
  final _files = <GeneratedFile>[
    GeneratedFile(path: 'lib/main.dart', content: _mainCode),
    GeneratedFile(path: 'lib/app.dart', content: _appCode),
    GeneratedFile(path: 'lib/pages/home_page.dart', content: _homeCode),
    GeneratedFile(path: 'lib/services/api_service.dart', content: _apiCode),
    GeneratedFile(path: 'pubspec.yaml', content: _pubspecCode),
  ];

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        _buildFileList(),
        const Divider(height: 1),
        Expanded(child: _buildCodeView()),
      ],
    );
  }

  Widget _buildFileList() {
    return SizedBox(
      height: 120,
      child: ListView(
        padding: const EdgeInsets.all(8),
        children: _files.map((f) => _buildFileItem(f)).toList(),
      ),
    );
  }

  Widget _buildFileItem(GeneratedFile file) {
    final isSelected = _selectedFile == file.path;
    return ListTile(
      dense: true,
      selected: isSelected,
      leading: Icon(_getFileIcon(file.path), size: 16),
      title: Text(file.path, style: const TextStyle(fontSize: 12)),
      onTap: () => setState(() => _selectedFile = file.path),
    );
  }

  IconData _getFileIcon(String path) {
    if (path.endsWith('.dart')) return Icons.code;
    if (path.endsWith('.yaml')) return Icons.settings;
    return Icons.insert_drive_file;
  }

  Widget _buildCodeView() {
    final file = _files.where((f) => f.path == _selectedFile).firstOrNull;
    if (file == null) {
      return const Center(child: Text('选择文件查看代码'));
    }
    return SingleChildScrollView(
      padding: const EdgeInsets.all(8),
      child: SelectableText(
        file.content,
        style: const TextStyle(fontFamily: 'monospace', fontSize: 11),
      ),
    );
  }
}

const _mainCode = '''import 'package:flutter/material.dart';
import 'app.dart';

void main() {
  runApp(const MyApp());
}
''';

const _appCode = '''import 'package:flutter/material.dart';
import 'pages/home_page.dart';

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: '我的应用',
      theme: ThemeData(useMaterial3: true),
      home: const HomePage(),
    );
  }
}
''';

const _homeCode = '''import 'package:flutter/material.dart';

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
''';

const _apiCode = '''import 'package:dio/dio.dart';

class ApiService {
  final Dio _dio = Dio();

  Future<dynamic> get(String path) async {
    return _dio.get(path);
  }
}
''';

const _pubspecCode = '''name: my_app
description: 由天枢IoT Studio生成

environment:
  sdk: ^3.0.0

dependencies:
  flutter:
    sdk: flutter
  dio: ^5.4.0
''';

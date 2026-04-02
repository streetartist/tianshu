import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../shared/providers/app_providers.dart';
import '../state/layout_canvas_notifier.dart';
import '../../codegen/code_generator.dart';
import '../../codegen/generators/page_generator.dart';
import '../../codegen/services/project_exporter.dart';

/// 代码预览对话框
class CodePreviewDialog extends ConsumerWidget {
  const CodePreviewDialog({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(layoutCanvasProvider);
    final layout = state.page;

    final generated = PageGenerator.generateFromLayout(
      layout,
      '${_toPascalCase(layout.name)}Page',
    );

    return AlertDialog(
      title: Row(
        children: [
          const Icon(Icons.code),
          const SizedBox(width: 8),
          const Text('生成代码预览'),
          const Spacer(),
          IconButton(
            icon: const Icon(Icons.copy),
            tooltip: '复制代码',
            onPressed: () {
              Clipboard.setData(ClipboardData(text: generated.content));
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('代码已复制到剪贴板')),
              );
            },
          ),
        ],
      ),
      content: SizedBox(
        width: 600,
        height: 500,
        child: _CodeView(code: generated.content),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text('关闭'),
        ),
        ElevatedButton.icon(
          icon: const Icon(Icons.folder_open),
          label: const Text('导出项目'),
          onPressed: () => _exportProject(context, ref),
        ),
      ],
    );
  }

  String _toPascalCase(String s) {
    return s.split(RegExp(r'[_\s]+')).map((w) =>
      w.isEmpty ? '' : w[0].toUpperCase() + w.substring(1)
    ).join();
  }

  Future<void> _exportProject(BuildContext context, WidgetRef ref) async {
    final project = ref.read(currentProjectProvider);
    if (project == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('请先创建项目')),
      );
      return;
    }

    // 生成完整项目
    final generator = CodeGenerator(project);
    final generatedProject = generator.generate();

    // 导出项目
    final success = await ProjectExporter.export(generatedProject);

    if (context.mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(success ? '项目导出成功' : '项目导出失败'),
        ),
      );
      if (success) Navigator.pop(context);
    }
  }
}

class _CodeView extends StatelessWidget {
  final String code;
  const _CodeView({required this.code});

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        color: const Color(0xFF1E1E1E),
        borderRadius: BorderRadius.circular(8),
      ),
      child: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: SelectableText(
          code,
          style: const TextStyle(
            fontFamily: 'monospace',
            fontSize: 12,
            color: Color(0xFFD4D4D4),
            height: 1.5,
          ),
        ),
      ),
    );
  }
}

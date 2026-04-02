import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../data/models/app_setting.dart';
import '../state/layout_canvas_notifier.dart';

/// 应用设置配置对话框
class AppSettingsDialog extends ConsumerStatefulWidget {
  const AppSettingsDialog({super.key});

  @override
  ConsumerState<AppSettingsDialog> createState() => _AppSettingsDialogState();
}

class _AppSettingsDialogState extends ConsumerState<AppSettingsDialog> {
  @override
  Widget build(BuildContext context) {
    final state = ref.watch(layoutCanvasProvider);
    final settings = state.appSettings.settings;

    return AlertDialog(
      title: const Text('应用设置配置'),
      content: SizedBox(
        width: 500,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              '配置生成App的设置项，可设置默认值和是否显示给用户',
              style: TextStyle(color: Colors.grey, fontSize: 12),
            ),
            const SizedBox(height: 16),
            // 设置项列表
            ConstrainedBox(
              constraints: const BoxConstraints(maxHeight: 400),
              child: ListView.builder(
                shrinkWrap: true,
                itemCount: settings.length,
                itemBuilder: (context, index) {
                  final setting = settings[index];
                  return _buildSettingTile(setting);
                },
              ),
            ),
            const Divider(),
            // 添加新设置项
            TextButton.icon(
              onPressed: _addSetting,
              icon: const Icon(Icons.add),
              label: const Text('添加设置项'),
            ),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text('关闭'),
        ),
      ],
    );
  }

  Widget _buildSettingTile(AppSetting setting) {
    return Card(
      child: ListTile(
        title: Text(setting.label),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('key: ${setting.key}', style: const TextStyle(fontSize: 11)),
            Text('默认值: ${setting.defaultValue.isEmpty ? "(空)" : setting.defaultValue}',
                style: const TextStyle(fontSize: 11)),
          ],
        ),
        trailing: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Tooltip(
              message: setting.visible ? '用户可见' : '用户不可见',
              child: IconButton(
                icon: Icon(
                  setting.visible ? Icons.visibility : Icons.visibility_off,
                  color: setting.visible ? Colors.green : Colors.grey,
                ),
                onPressed: () => _toggleVisible(setting),
              ),
            ),
            IconButton(
              icon: const Icon(Icons.edit, size: 20),
              onPressed: () => _editSetting(setting),
            ),
            if (setting.key != 'server_url')
              IconButton(
                icon: const Icon(Icons.delete_outline, size: 20),
                onPressed: () => ref.read(layoutCanvasProvider.notifier)
                    .removeAppSetting(setting.key),
              ),
          ],
        ),
      ),
    );
  }

  void _toggleVisible(AppSetting setting) {
    ref.read(layoutCanvasProvider.notifier).updateAppSetting(
      setting.key,
      visible: !setting.visible,
    );
  }

  void _editSetting(AppSetting setting) {
    final labelCtrl = TextEditingController(text: setting.label);
    final valueCtrl = TextEditingController(text: setting.defaultValue);
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text('编辑 ${setting.key}'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: labelCtrl,
              decoration: const InputDecoration(
                labelText: '显示名称',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: valueCtrl,
              decoration: const InputDecoration(
                labelText: '默认值',
                border: OutlineInputBorder(),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('取消')),
          ElevatedButton(
            onPressed: () {
              ref.read(layoutCanvasProvider.notifier).updateAppSetting(
                setting.key,
                label: labelCtrl.text.trim(),
                defaultValue: valueCtrl.text.trim(),
              );
              Navigator.pop(ctx);
            },
            child: const Text('保存'),
          ),
        ],
      ),
    );
  }

  void _addSetting() {
    final keyCtrl = TextEditingController();
    final labelCtrl = TextEditingController();
    final valueCtrl = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('添加设置项'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: keyCtrl,
              decoration: const InputDecoration(
                labelText: 'Key (英文标识)',
                hintText: '如: api_key',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: labelCtrl,
              decoration: const InputDecoration(
                labelText: '显示名称',
                hintText: '如: API密钥',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: valueCtrl,
              decoration: const InputDecoration(
                labelText: '默认值',
                border: OutlineInputBorder(),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('取消'),
          ),
          ElevatedButton(
            onPressed: () {
              final key = keyCtrl.text.trim();
              if (key.isEmpty) return;
              ref.read(layoutCanvasProvider.notifier).addAppSetting(
                AppSetting(
                  key: key,
                  label: labelCtrl.text.trim(),
                  defaultValue: valueCtrl.text.trim(),
                ),
              );
              Navigator.pop(ctx);
            },
            child: const Text('添加'),
          ),
        ],
      ),
    );
  }
}
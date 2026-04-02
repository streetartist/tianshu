import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../state/layout_canvas_notifier.dart';

/// 设备槽位管理对话框
class SlotManagerDialog extends ConsumerStatefulWidget {
  const SlotManagerDialog({super.key});

  @override
  ConsumerState<SlotManagerDialog> createState() => _SlotManagerDialogState();
}

class _SlotManagerDialogState extends ConsumerState<SlotManagerDialog> {
  final _nameController = TextEditingController();

  @override
  void dispose() {
    _nameController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(layoutCanvasProvider);
    final slots = state.deviceConfig.slots;
    final enableDeviceManagement = state.deviceConfig.enableDeviceManagement;

    return AlertDialog(
      title: const Text('设备槽位管理'),
      content: SizedBox(
        width: 400,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              '槽位是设计时的设备占位符，运行时绑定实际设备',
              style: TextStyle(color: Colors.grey, fontSize: 12),
            ),
            const SizedBox(height: 12),
            // 设备管理开关
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.blue.shade50,
                borderRadius: BorderRadius.circular(8),
              ),
              child: Row(
                children: [
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Text('启用设备管理', style: TextStyle(fontWeight: FontWeight.bold)),
                        Text(
                          '允许用户在App中切换设备',
                          style: TextStyle(fontSize: 12, color: Colors.grey.shade700),
                        ),
                      ],
                    ),
                  ),
                  Switch(
                    value: enableDeviceManagement,
                    onChanged: (v) => ref.read(layoutCanvasProvider.notifier).setDeviceManagementEnabled(v),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),
            // 添加新槽位
            Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _nameController,
                    decoration: const InputDecoration(
                      hintText: '新槽位名称',
                      isDense: true,
                      border: OutlineInputBorder(),
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                ElevatedButton(
                  onPressed: _addSlot,
                  child: const Text('添加'),
                ),
              ],
            ),
            const SizedBox(height: 16),
            const Divider(),
            // 槽位列表
            ConstrainedBox(
              constraints: const BoxConstraints(maxHeight: 300),
              child: ListView.builder(
                shrinkWrap: true,
                itemCount: slots.length,
                itemBuilder: (context, index) {
                  final slot = slots[index];
                  final isDefault = slot.id == 'default';
                  return ListTile(
                    leading: Icon(
                      isDefault ? Icons.star : Icons.devices,
                      color: isDefault ? Colors.amber : null,
                    ),
                    title: Text(slot.name),
                    subtitle: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        if (slot.defaultDeviceId.isNotEmpty)
                          Text('设备ID: ${slot.defaultDeviceId}',
                            style: const TextStyle(fontSize: 11, color: Colors.green)),
                        if (slot.defaultDeviceId.isEmpty)
                          const Text('未设置默认设备',
                            style: TextStyle(fontSize: 11, color: Colors.grey)),
                      ],
                    ),
                    trailing: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        IconButton(
                          icon: const Icon(Icons.edit, size: 20),
                          onPressed: () => _editSlot(slot),
                          tooltip: '设置设备ID',
                        ),
                        if (!isDefault)
                          IconButton(
                            icon: const Icon(Icons.delete_outline, size: 20),
                            onPressed: () => _deleteSlot(slot.id),
                          ),
                      ],
                    ),
                  );
                },
              ),
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

  void _addSlot() {
    final name = _nameController.text.trim();
    if (name.isEmpty) return;
    ref.read(layoutCanvasProvider.notifier).addSlot(name);
    _nameController.clear();
  }

  void _deleteSlot(String slotId) {
    ref.read(layoutCanvasProvider.notifier).removeSlot(slotId);
  }

  void _editSlot(slot) {
    final idController = TextEditingController(text: slot.defaultDeviceId);
    final keyController = TextEditingController(text: slot.defaultSecretKey);
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text('设置 ${slot.name}'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: idController,
              decoration: const InputDecoration(
                labelText: '设备ID',
                hintText: '数据库中的设备ID',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: keyController,
              decoration: const InputDecoration(
                labelText: 'Secret Key',
                hintText: '设备密钥',
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
              ref.read(layoutCanvasProvider.notifier).updateSlotDevice(
                slot.id,
                idController.text.trim(),
                keyController.text.trim(),
              );
              Navigator.pop(ctx);
            },
            child: const Text('保存'),
          ),
        ],
      ),
    );
  }
}

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../data/models/layout_models.dart';
import '../state/layout_canvas_notifier.dart';

/// 布局属性面板
class LayoutPropertyPanel extends ConsumerWidget {
  const LayoutPropertyPanel({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final canvasState = ref.watch(layoutCanvasProvider);
    final selectedItem = canvasState.page.children
        .where((c) => c.id == canvasState.selectedItemId)
        .firstOrNull;

    return Container(
      width: 280,
      decoration: BoxDecoration(
        border: Border(
          left: BorderSide(color: Theme.of(context).dividerColor),
        ),
      ),
      child: Column(
        children: [
          _buildHeader(selectedItem, ref),
          const Divider(height: 1),
          Expanded(child: _buildContent(selectedItem, ref)),
        ],
      ),
    );
  }

  Widget _buildHeader(LayoutItem? item, WidgetRef ref) {
    if (item == null) return const SizedBox(height: 48);
    return Padding(
      padding: const EdgeInsets.all(8),
      child: Row(
        children: [
          Expanded(child: Text(item.type, style: const TextStyle(fontWeight: FontWeight.bold))),
          IconButton(
            icon: const Icon(Icons.arrow_upward, size: 20),
            onPressed: () => ref.read(layoutCanvasProvider.notifier).moveItem(item.id, -1),
          ),
          IconButton(
            icon: const Icon(Icons.arrow_downward, size: 20),
            onPressed: () => ref.read(layoutCanvasProvider.notifier).moveItem(item.id, 1),
          ),
          IconButton(
            icon: const Icon(Icons.delete, size: 20),
            onPressed: () => ref.read(layoutCanvasProvider.notifier).deleteItem(item.id),
          ),
        ],
      ),
    );
  }

  Widget _buildContent(LayoutItem? item, WidgetRef ref) {
    if (item == null) {
      return const Center(child: Text('选择控件编辑属性'));
    }
    return ListView(
      padding: const EdgeInsets.all(16),
      children: _buildPropertyFields(item, ref),
    );
  }

  List<Widget> _buildPropertyFields(LayoutItem item, WidgetRef ref) {
    return item.properties.entries.map((e) {
      return Padding(
        padding: const EdgeInsets.only(bottom: 12),
        child: _buildField(item.id, e.key, e.value, ref),
      );
    }).toList();
  }

  Widget _buildField(String id, String key, dynamic value, WidgetRef ref) {
    if (value is bool) {
      return SwitchListTile(
        title: Text(key),
        value: value,
        contentPadding: EdgeInsets.zero,
        onChanged: (v) => ref.read(layoutCanvasProvider.notifier)
            .updateItemProperty(id, key, v),
      );
    }
    if (value is num) {
      return TextField(
        decoration: InputDecoration(labelText: key, border: const OutlineInputBorder()),
        controller: TextEditingController(text: value.toString()),
        keyboardType: TextInputType.number,
        onSubmitted: (v) => ref.read(layoutCanvasProvider.notifier)
            .updateItemProperty(id, key, num.tryParse(v) ?? value),
      );
    }
    return TextField(
      decoration: InputDecoration(labelText: key, border: const OutlineInputBorder()),
      controller: TextEditingController(text: value.toString()),
      onSubmitted: (v) => ref.read(layoutCanvasProvider.notifier)
          .updateItemProperty(id, key, v),
    );
  }
}

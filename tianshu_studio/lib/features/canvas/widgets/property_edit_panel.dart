import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../data/models/layout_models.dart';
import '../state/layout_canvas_notifier.dart';

/// 属性编辑面板
class PropertyEditPanel extends ConsumerWidget {
  const PropertyEditPanel({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(layoutCanvasProvider);
    final item = _findItem(state.page.children, state.selectedItemId);

    return Container(
      width: 300,
      decoration: BoxDecoration(
        border: Border(left: BorderSide(color: Theme.of(context).dividerColor)),
      ),
      child: item == null
          ? const Center(child: Text('选择控件编辑属性'))
          : _PropertyEditor(item: item),
    );
  }

  /// 递归查找控件（包括容器内的子控件）
  LayoutItem? _findItem(List<LayoutItem> items, String? id) {
    if (id == null) return null;
    for (final item in items) {
      if (item.id == id) return item;
      if (item.children != null) {
        final found = _findItem(item.children!, id);
        if (found != null) return found;
      }
    }
    return null;
  }
}

class _PropertyEditor extends ConsumerStatefulWidget {
  final LayoutItem item;
  const _PropertyEditor({required this.item});

  @override
  ConsumerState<_PropertyEditor> createState() => _PropertyEditorState();
}

class _PropertyEditorState extends ConsumerState<_PropertyEditor> {
  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        _buildHeader(),
        const Divider(height: 1),
        Expanded(child: _buildPropertyList()),
      ],
    );
  }

  Widget _buildHeader() {
    return Container(
      padding: const EdgeInsets.all(12),
      child: Row(
        children: [
          Expanded(
            child: Text(widget.item.type,
              style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 16)),
          ),
          IconButton(
            icon: const Icon(Icons.delete_outline),
            onPressed: () => ref.read(layoutCanvasProvider.notifier)
                .deleteItem(widget.item.id),
          ),
        ],
      ),
    );
  }

  Widget _buildPropertyList() {
    final isContainer = widget.item.type == 'container';
    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        _buildSection('基本属性', _buildBasicProps()),
        if (isContainer) ...[
          const SizedBox(height: 16),
          _buildSection('容器设置', _buildContainerProps()),
        ],
        if (!isContainer) ...[
          const SizedBox(height: 16),
          _buildSection('数据绑定', _buildBindingProps()),
        ],
      ],
    );
  }

  Widget _buildSection(String title, List<Widget> children) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(title, style: TextStyle(color: Colors.grey[600], fontWeight: FontWeight.bold)),
        const SizedBox(height: 8),
        ...children,
      ],
    );
  }

  List<Widget> _buildBasicProps() {
    final basicKeys = ['label', 'text', 'unit', 'min', 'max', 'options', 'title'];
    return widget.item.properties.entries
        .where((e) => basicKeys.contains(e.key))
        .map((e) => _buildField(e.key, e.value))
        .toList();
  }

  List<Widget> _buildContainerProps() {
    final props = widget.item.properties;
    return [
      // 布局类型选择
      _buildLayoutSelector(props['layout'] ?? 'column'),
      // 显示标题开关
      _buildSwitch('showTitle', '显示标题', props['showTitle'] ?? true),
      // 显示边框开关
      _buildSwitch('showBorder', '显示边框', props['showBorder'] ?? true),
    ];
  }

  Widget _buildLayoutSelector(String current) {
    final layouts = [
      ('column', '纵向'),
      ('row', '横向'),
    ];
    // 如果当前值不在列表中，默认使用column
    final validCurrent = layouts.any((l) => l.$1 == current) ? current : 'column';
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: DropdownButtonFormField<String>(
        value: validCurrent,
        decoration: const InputDecoration(
          labelText: '内部布局',
          border: OutlineInputBorder(),
          isDense: true,
        ),
        items: layouts.map((l) => DropdownMenuItem(
          value: l.$1,
          child: Text(l.$2),
        )).toList(),
        onChanged: (v) {
          if (v != null) {
            ref.read(layoutCanvasProvider.notifier)
                .updateItemProperty(widget.item.id, 'layout', v);
          }
        },
      ),
    );
  }

  Widget _buildSwitch(String key, String label, bool value) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label),
          Switch(
            value: value,
            onChanged: (v) {
              ref.read(layoutCanvasProvider.notifier)
                  .updateItemProperty(widget.item.id, key, v);
            },
          ),
        ],
      ),
    );
  }

  List<Widget> _buildBindingProps() {
    final props = widget.item.properties;
    final widgets = <Widget>[];
    final isButton = widget.item.type == 'button';

    // 设备槽位选择
    if (props.containsKey('slotId')) {
      widgets.add(_buildSlotSelector(props['slotId'] ?? 'default'));
    }

    // 命令名称
    if (props.containsKey('command')) {
      widgets.add(_buildField('command', props['command'] ?? ''));
    }

    // 按钮特有：命令值和类型
    if (isButton) {
      final commandType = props['commandType'] ?? 'none';
      widgets.add(_buildCommandTypeSelector(commandType));
      // 只有非"无参数"时才显示发送值输入框
      if (commandType != 'none') {
        widgets.add(_buildField('commandValue', props['commandValue'] ?? ''));
      }
    }

    // 数据字段（显示控件用）
    if (props.containsKey('fieldName')) {
      widgets.add(_buildField('fieldName', props['fieldName'] ?? ''));
    }

    return widgets;
  }

  Widget _buildCommandTypeSelector(String current) {
    final types = [
      ('none', '无参数'),
      ('bool', '布尔值'),
      ('number', '数字'),
      ('string', '字符串'),
      ('json', 'JSON对象'),
    ];
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: DropdownButtonFormField<String>(
        value: types.any((t) => t.$1 == current) ? current : 'none',
        decoration: const InputDecoration(
          labelText: '值类型',
          border: OutlineInputBorder(),
          isDense: true,
        ),
        items: types.map((t) => DropdownMenuItem(
          value: t.$1,
          child: Text(t.$2),
        )).toList(),
        onChanged: (v) {
          if (v != null) {
            ref.read(layoutCanvasProvider.notifier)
                .updateItemProperty(widget.item.id, 'commandType', v);
          }
        },
      ),
    );
  }

  Widget _buildSlotSelector(String currentSlotId) {
    final state = ref.watch(layoutCanvasProvider);
    final slots = state.deviceConfig.slots;

    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: DropdownButtonFormField<String>(
        value: currentSlotId,
        decoration: const InputDecoration(
          labelText: '设备槽位',
          border: OutlineInputBorder(),
          isDense: true,
        ),
        items: slots.map((slot) => DropdownMenuItem(
          value: slot.id,
          child: Text(slot.id == 'default' ? '${slot.name} (自动)' : slot.name),
        )).toList(),
        onChanged: (v) {
          if (v != null) {
            ref.read(layoutCanvasProvider.notifier)
                .updateItemProperty(widget.item.id, 'slotId', v);
          }
        },
      ),
    );
  }

  Widget _buildField(String key, dynamic value) {
    final labels = {
      'label': '显示标签', 'text': '按钮文字', 'unit': '单位',
      'min': '最小值', 'max': '最大值', 'options': '选项',
      'fieldName': '数据字段', 'command': '命令名称',
      'commandValue': '发送值', 'title': '标题',
    };
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: _PropertyTextField(
        label: labels[key] ?? key,
        initialValue: value.toString(),
        onChanged: (v) {
          final newVal = num.tryParse(v) ?? v;
          ref.read(layoutCanvasProvider.notifier)
              .updateItemProperty(widget.item.id, key, newVal);
        },
      ),
    );
  }
}

/// 属性输入框（解决光标位置问题）
class _PropertyTextField extends StatefulWidget {
  final String label;
  final String initialValue;
  final ValueChanged<String> onChanged;

  const _PropertyTextField({
    required this.label,
    required this.initialValue,
    required this.onChanged,
  });

  @override
  State<_PropertyTextField> createState() => _PropertyTextFieldState();
}

class _PropertyTextFieldState extends State<_PropertyTextField> {
  late TextEditingController _controller;

  @override
  void initState() {
    super.initState();
    _controller = TextEditingController(text: widget.initialValue);
  }

  @override
  void didUpdateWidget(_PropertyTextField oldWidget) {
    super.didUpdateWidget(oldWidget);
    // 只有当外部值变化且不是由本输入框引起时才更新
    if (widget.initialValue != oldWidget.initialValue &&
        widget.initialValue != _controller.text) {
      _controller.text = widget.initialValue;
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return TextField(
      controller: _controller,
      decoration: InputDecoration(
        labelText: widget.label,
        border: const OutlineInputBorder(),
        isDense: true,
      ),
      onChanged: widget.onChanged,
    );
  }
}

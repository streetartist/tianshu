import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../data/models/layout_models.dart';
import '../../widgets/runtime/runtime_widgets.dart';
import '../state/layout_canvas_notifier.dart';

/// 画布控件渲染器 - 使用运行时控件实现所见即所得
class CanvasWidgetRenderer {
  static Widget render(LayoutItem item) {
    final props = item.properties;

    switch (item.type) {
      case 'numberDisplay':
        return NumberDisplayWidget(
          label: props['label'] ?? '数值',
          value: props['value'],
          unit: props['unit'] ?? '',
        );

      case 'textDisplay':
        return _buildTextDisplay(props);

      case 'statusLight':
        return StatusLightWidget(
          label: props['label'] ?? '状态',
          status: props['status'] ?? false,
        );

      case 'progressBar':
        return ProgressBarWidget(
          label: props['label'] ?? '进度',
          value: (props['value'] ?? 0).toDouble(),
          min: (props['min'] ?? 0).toDouble(),
          max: (props['max'] ?? 100).toDouble(),
        );

      case 'switchButton':
        return SwitchWidget(
          label: props['label'] ?? '开关',
          value: props['value'] ?? false,
        );

      case 'button':
        return ButtonWidget(text: props['text'] ?? '按钮');

      case 'slider':
        return SliderWidget(
          label: props['label'] ?? '滑块',
          value: (props['value'] ?? 50).toDouble(),
          min: (props['min'] ?? 0).toDouble(),
          max: (props['max'] ?? 100).toDouble(),
        );

      case 'gauge':
        return GaugeWidget(
          label: props['label'] ?? '仪表',
          value: (props['value'] ?? 0).toDouble(),
          min: (props['min'] ?? 0).toDouble(),
          max: (props['max'] ?? 100).toDouble(),
          unit: props['unit'] ?? '',
        );

      case 'dropdown':
        final optStr = props['options'] ?? '';
        final opts = optStr.toString().split(',').where((s) => s.isNotEmpty).toList();
        return DropdownWidget(
          label: props['label'] ?? '选择',
          value: props['value'] ?? '',
          options: opts,
        );

      case 'numberInput':
        return NumberInputWidget(
          label: props['label'] ?? '数值',
          value: (props['value'] ?? 0).toDouble(),
          min: (props['min'] ?? 0).toDouble(),
          max: (props['max'] ?? 100).toDouble(),
          step: (props['step'] ?? 1).toDouble(),
        );

      case 'lineChart':
        return LineChartWidget(
          title: props['title'] ?? '趋势图',
        );

      case 'barChart':
        return BarChartWidget(
          title: props['title'] ?? '统计图',
        );

      case 'container':
        return _buildContainer(item);

      default:
        return _buildDefault(item.type);
    }
  }

  /// 渲染容器及其子控件
  static Widget _buildContainer(LayoutItem item) {
    final props = item.properties;
    final layoutStr = props['layout'] ?? 'column';
    final layout = ContainerLayout.values.firstWhere(
      (l) => l.name == layoutStr,
      orElse: () => ContainerLayout.column,
    );

    // 递归渲染子控件（不可选中）
    final children = item.children?.map((child) => render(child)).toList() ?? [];

    return ContainerWidget(
      title: props['title'] ?? '分组',
      showBorder: props['showBorder'] ?? true,
      showTitle: props['showTitle'] ?? true,
      layout: layout,
      gridColumns: props['gridColumns'] ?? 2,
      children: children,
    );
  }

  /// 渲染容器（子控件可选中、可拖动）
  static Widget renderContainer(LayoutItem item, WidgetRef ref) {
    final props = item.properties;
    final layoutStr = props['layout'] ?? 'column';
    final layout = ContainerLayout.values.firstWhere(
      (l) => l.name == layoutStr,
      orElse: () => ContainerLayout.column,
    );
    final state = ref.watch(layoutCanvasProvider);
    final selectedId = state.selectedItemId;

    // 渲染可选中、可拖动的子控件
    final children = item.children?.map((child) {
      final isSelected = child.id == selectedId;
      return _buildDraggableChild(child, isSelected, ref);
    }).toList() ?? [];

    return ContainerWidget(
      title: props['title'] ?? '分组',
      showBorder: props['showBorder'] ?? true,
      showTitle: props['showTitle'] ?? true,
      layout: layout,
      gridColumns: props['gridColumns'] ?? 2,
      children: children,
    );
  }

  /// 构建可拖动的子控件
  static Widget _buildDraggableChild(LayoutItem child, bool isSelected, WidgetRef ref) {
    return Draggable<LayoutItem>(
      data: child,
      feedback: Material(
        elevation: 8,
        borderRadius: BorderRadius.circular(8),
        child: Opacity(
          opacity: 0.8,
          child: SizedBox(width: 150, child: render(child)),
        ),
      ),
      childWhenDragging: Opacity(
        opacity: 0.3,
        child: _buildChildContent(child, isSelected, ref),
      ),
      child: _buildChildContent(child, isSelected, ref),
    );
  }

  static Widget _buildChildContent(LayoutItem child, bool isSelected, WidgetRef ref) {
    return GestureDetector(
      onTap: () => ref.read(layoutCanvasProvider.notifier).selectItem(child.id),
      child: Container(
        margin: const EdgeInsets.only(bottom: 4),
        decoration: BoxDecoration(
          border: Border.all(
            color: isSelected ? Colors.blue : Colors.transparent,
            width: 2,
          ),
          borderRadius: BorderRadius.circular(6),
        ),
        child: Stack(
          children: [
            render(child),
            // 拖动手柄
            Positioned(
              top: 2,
              left: 2,
              child: Container(
                padding: const EdgeInsets.all(2),
                decoration: BoxDecoration(
                  color: Colors.black.withValues(alpha: 0.5),
                  borderRadius: BorderRadius.circular(3),
                ),
                child: const Icon(Icons.drag_indicator, color: Colors.white, size: 12),
              ),
            ),
          ],
        ),
      ),
    );
  }

  static Widget _buildTextDisplay(Map<String, dynamic> props) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(color: Colors.black.withOpacity(0.05), blurRadius: 10),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(props['label'] ?? '文本',
              style: TextStyle(fontSize: 14, color: Colors.grey[600])),
          const SizedBox(height: 4),
          Text(props['text'] ?? '--', style: const TextStyle(fontSize: 18)),
        ],
      ),
    );
  }

  static Widget _buildDefault(String type) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.grey[100],
        borderRadius: BorderRadius.circular(12),
      ),
      child: Center(child: Text(type)),
    );
  }
}

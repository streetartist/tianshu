import 'package:flutter/material.dart';
import '../../widgets/widget_type.dart';

/// 控件面板 - 左侧控件库
class WidgetPalette extends StatelessWidget {
  const WidgetPalette({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 200,
      decoration: BoxDecoration(
        border: Border(
          right: BorderSide(color: Theme.of(context).dividerColor),
        ),
      ),
      child: ListView(
        children: [
          _buildCategory('显示控件', _getWidgetsByCategory('display')),
          _buildCategory('输入控件', _getWidgetsByCategory('input')),
          _buildCategory('控制控件', _getWidgetsByCategory('control')),
          _buildCategory('图表控件', _getWidgetsByCategory('chart')),
          _buildCategory('布局控件', _getWidgetsByCategory('layout')),
        ],
      ),
    );
  }

  List<WidgetType> _getWidgetsByCategory(String category) {
    return WidgetType.values.where((t) => t.category == category).toList();
  }

  Widget _buildCategory(String title, List<WidgetType> types) {
    if (types.isEmpty) return const SizedBox.shrink();
    return ExpansionTile(
      title: Text(title, style: const TextStyle(fontSize: 14)),
      initiallyExpanded: true,
      children: types.map((t) => _buildDraggableItem(t)).toList(),
    );
  }

  Widget _buildDraggableItem(WidgetType type) {
    return Draggable<WidgetType>(
      data: type,
      feedback: Material(
        elevation: 4,
        child: Container(
          padding: const EdgeInsets.all(8),
          color: Colors.blue.shade100,
          child: Text(type.label),
        ),
      ),
      child: ListTile(
        dense: true,
        leading: Icon(_getIcon(type), size: 20),
        title: Text(type.label, style: const TextStyle(fontSize: 13)),
      ),
    );
  }

  IconData _getIcon(WidgetType type) {
    switch (type) {
      case WidgetType.numberDisplay: return Icons.numbers;
      case WidgetType.textDisplay: return Icons.text_fields;
      case WidgetType.statusLight: return Icons.lightbulb;
      case WidgetType.progressBar: return Icons.linear_scale;
      case WidgetType.gauge: return Icons.speed;
      case WidgetType.numberInput: return Icons.input;
      case WidgetType.slider: return Icons.tune;
      case WidgetType.dropdown: return Icons.arrow_drop_down;
      case WidgetType.switchButton: return Icons.toggle_on;
      case WidgetType.button: return Icons.smart_button;
      case WidgetType.lineChart: return Icons.show_chart;
      case WidgetType.barChart: return Icons.bar_chart;
      case WidgetType.container: return Icons.crop_square;
    }
  }
}

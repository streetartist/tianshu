import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../data/models/layout_models.dart';
import '../../widgets/runtime/runtime_widgets.dart';
import '../state/layout_canvas_notifier.dart';

/// 设备预览尺寸
enum PreviewDevice {
  phone('手机', 375, 667),
  phoneLarge('大屏手机', 414, 896),
  tablet('平板', 768, 1024),
  desktop('桌面', 1280, 800);

  final String label;
  final double width;
  final double height;

  const PreviewDevice(this.label, this.width, this.height);
}

/// 预览对话框
class PreviewDialog extends ConsumerStatefulWidget {
  const PreviewDialog({super.key});

  @override
  ConsumerState<PreviewDialog> createState() => _PreviewDialogState();
}

class _PreviewDialogState extends ConsumerState<PreviewDialog> {
  PreviewDevice _device = PreviewDevice.phone;
  bool _isLandscape = false;

  @override
  Widget build(BuildContext context) {
    final canvasState = ref.watch(layoutCanvasProvider);
    final page = canvasState.page;

    return Dialog(
      backgroundColor: Colors.grey.shade900,
      insetPadding: const EdgeInsets.all(16),
      child: SizedBox(
        width: MediaQuery.of(context).size.width * 0.9,
        height: MediaQuery.of(context).size.height * 0.9,
        child: Column(
          children: [
            _buildToolbar(),
            Expanded(child: _buildPreviewArea(page)),
          ],
        ),
      ),
    );
  }

  Widget _buildToolbar() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      decoration: BoxDecoration(
        color: Colors.grey.shade800,
        borderRadius: const BorderRadius.vertical(top: Radius.circular(12)),
      ),
      child: Row(
        children: [
          const Text('预览', style: TextStyle(color: Colors.white, fontSize: 16)),
          const SizedBox(width: 24),
          // 设备选择
          ...PreviewDevice.values.map((d) => Padding(
            padding: const EdgeInsets.only(right: 8),
            child: _buildDeviceButton(d),
          )),
          const SizedBox(width: 16),
          // 横竖屏切换
          IconButton(
            icon: Icon(
              _isLandscape ? Icons.stay_current_landscape : Icons.stay_current_portrait,
              color: Colors.white,
            ),
            onPressed: () => setState(() => _isLandscape = !_isLandscape),
            tooltip: _isLandscape ? '竖屏' : '横屏',
          ),
          const Spacer(),
          IconButton(
            icon: const Icon(Icons.close, color: Colors.white),
            onPressed: () => Navigator.of(context).pop(),
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceButton(PreviewDevice device) {
    final isSelected = _device == device;
    return TextButton(
      style: TextButton.styleFrom(
        backgroundColor: isSelected ? Colors.blue : Colors.transparent,
        foregroundColor: Colors.white,
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      ),
      onPressed: () => setState(() => _device = device),
      child: Text(device.label),
    );
  }

  Widget _buildPreviewArea(PageLayout page) {
    final width = _isLandscape ? _device.height : _device.width;
    final height = _isLandscape ? _device.width : _device.height;

    return Center(
      child: Container(
        width: width + 20,
        height: height + 40,
        decoration: BoxDecoration(
          color: Colors.black,
          borderRadius: BorderRadius.circular(24),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withValues(alpha: 0.5),
              blurRadius: 20,
              spreadRadius: 5,
            ),
          ],
        ),
        padding: const EdgeInsets.all(10),
        child: ClipRRect(
          borderRadius: BorderRadius.circular(16),
          child: Container(
            color: Colors.white,
            child: _buildPageContent(page),
          ),
        ),
      ),
    );
  }

  Widget _buildPageContent(PageLayout page) {
    if (page.children.isEmpty) {
      return const Center(
        child: Text('暂无内容', style: TextStyle(color: Colors.grey)),
      );
    }

    return ListView.builder(
      padding: const EdgeInsets.all(12),
      itemCount: page.children.length,
      itemBuilder: (context, index) {
        return Padding(
          padding: const EdgeInsets.only(bottom: 8),
          child: _PreviewWidgetRenderer.render(page.children[index]),
        );
      },
    );
  }
}

/// 预览控件渲染器 - 纯净渲染，无编辑元素
class _PreviewWidgetRenderer {
  static Widget render(LayoutItem item) {
    final props = item.properties;

    switch (item.type) {
      case 'numberDisplay':
        return NumberDisplayWidget(
          label: props['label'] ?? '数值',
          value: _getMockValue(props),
          unit: props['unit'] ?? '',
        );

      case 'textDisplay':
        return _buildTextDisplay(props);

      case 'statusLight':
        return StatusLightWidget(
          label: props['label'] ?? '状态',
          status: true, // 预览时显示在线
        );

      case 'progressBar':
        return ProgressBarWidget(
          label: props['label'] ?? '进度',
          value: _getMockValue(props, defaultVal: 65),
          min: (props['min'] ?? 0).toDouble(),
          max: (props['max'] ?? 100).toDouble(),
        );

      case 'switchButton':
        return SwitchWidget(
          label: props['label'] ?? '开关',
          value: true,
        );

      case 'button':
        return ButtonWidget(text: props['text'] ?? '按钮');

      case 'slider':
        return SliderWidget(
          label: props['label'] ?? '滑块',
          value: _getMockValue(props, defaultVal: 50),
          min: (props['min'] ?? 0).toDouble(),
          max: (props['max'] ?? 100).toDouble(),
        );

      case 'gauge':
        return GaugeWidget(
          label: props['label'] ?? '仪表',
          value: _getMockValue(props, defaultVal: 42),
          min: (props['min'] ?? 0).toDouble(),
          max: (props['max'] ?? 100).toDouble(),
          unit: props['unit'] ?? '',
        );

      case 'dropdown':
        final optStr = props['options'] ?? '';
        final opts = optStr.toString().split(',').where((s) => s.isNotEmpty).toList();
        return DropdownWidget(
          label: props['label'] ?? '选择',
          value: opts.isNotEmpty ? opts.first : '',
          options: opts,
        );

      case 'numberInput':
        return NumberInputWidget(
          label: props['label'] ?? '数值',
          value: _getMockValue(props, defaultVal: 25),
          min: (props['min'] ?? 0).toDouble(),
          max: (props['max'] ?? 100).toDouble(),
          step: (props['step'] ?? 1).toDouble(),
        );

      case 'lineChart':
        return LineChartWidget(
          title: props['title'] ?? '趋势图',
          data: const [20, 35, 28, 45, 38, 52, 48, 60, 55, 70],
        );

      case 'barChart':
        return BarChartWidget(
          title: props['title'] ?? '统计图',
          data: const [30, 45, 28, 52, 38, 42, 35],
        );

      case 'container':
        return _buildContainer(item);

      default:
        return Container(
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: Colors.grey[100],
            borderRadius: BorderRadius.circular(12),
          ),
          child: Center(child: Text(item.type)),
        );
    }
  }

  static double _getMockValue(Map<String, dynamic> props, {double defaultVal = 0}) {
    if (props['value'] != null) {
      return (props['value'] as num).toDouble();
    }
    return defaultVal;
  }

  static Widget _buildTextDisplay(Map<String, dynamic> props) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(color: Colors.black.withValues(alpha: 0.05), blurRadius: 10),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(props['label'] ?? '文本',
              style: TextStyle(fontSize: 14, color: Colors.grey[600])),
          const SizedBox(height: 4),
          Text(props['text'] ?? '示例文本', style: const TextStyle(fontSize: 18)),
        ],
      ),
    );
  }

  static Widget _buildContainer(LayoutItem item) {
    final props = item.properties;
    final layoutStr = props['layout'] ?? 'column';
    final layout = ContainerLayout.values.firstWhere(
      (l) => l.name == layoutStr,
      orElse: () => ContainerLayout.column,
    );

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
}

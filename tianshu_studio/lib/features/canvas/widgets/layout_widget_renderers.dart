import 'package:flutter/material.dart';
import '../../../data/models/layout_models.dart';

/// 布局控件渲染器
class LayoutWidgetRenderers {
  static Widget render(LayoutItem item) {
    switch (item.type) {
      case 'numberDisplay':
        return _renderNumberDisplay(item);
      case 'textDisplay':
        return _renderTextDisplay(item);
      case 'statusLight':
        return _renderStatusLight(item);
      case 'progressBar':
        return _renderProgressBar(item);
      case 'switchButton':
        return _renderSwitch(item);
      case 'button':
        return _renderButton(item);
      case 'gauge':
        return _renderGauge(item);
      case 'slider':
        return _renderSlider(item);
      default:
        return _renderDefault(item);
    }
  }

  static Widget _renderNumberDisplay(LayoutItem item) {
    final value = item.properties['value'] ?? 0;
    final unit = item.properties['unit'] ?? '';
    final label = item.properties['label'] ?? '数值';
    return _buildCard(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(label, style: const TextStyle(color: Colors.grey)),
          Text('$value$unit', style: const TextStyle(fontSize: 28)),
        ],
      ),
    );
  }

  static Widget _renderTextDisplay(LayoutItem item) {
    return _buildCard(
      child: Text(item.properties['text'] ?? '文本'),
    );
  }

  static Widget _renderStatusLight(LayoutItem item) {
    final status = item.properties['status'] ?? false;
    final label = item.properties['label'] ?? '状态';
    return _buildCard(
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label),
          Container(
            width: 16, height: 16,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: status ? Colors.green : Colors.grey,
            ),
          ),
        ],
      ),
    );
  }

  static Widget _renderProgressBar(LayoutItem item) {
    final value = (item.properties['value'] ?? 50) / 100;
    final label = item.properties['label'] ?? '进度';
    return _buildCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(label),
          const SizedBox(height: 8),
          LinearProgressIndicator(value: value.toDouble()),
        ],
      ),
    );
  }

  static Widget _renderSwitch(LayoutItem item) {
    final label = item.properties['label'] ?? '开关';
    return _buildCard(
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label),
          Switch(value: item.properties['value'] ?? false, onChanged: null),
        ],
      ),
    );
  }

  static Widget _renderButton(LayoutItem item) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: ElevatedButton(
        onPressed: null,
        style: ElevatedButton.styleFrom(
          minimumSize: const Size.fromHeight(48),
        ),
        child: Text(item.properties['text'] ?? '按钮'),
      ),
    );
  }

  static Widget _renderGauge(LayoutItem item) {
    final value = item.properties['value'] ?? 50;
    final label = item.properties['label'] ?? '仪表';
    return _buildCard(
      child: Column(
        children: [
          Text(label),
          const SizedBox(height: 8),
          SizedBox(
            width: 80, height: 80,
            child: CircularProgressIndicator(
              value: value / 100,
              strokeWidth: 8,
            ),
          ),
          Text('$value%'),
        ],
      ),
    );
  }

  static Widget _renderSlider(LayoutItem item) {
    final value = (item.properties['value'] ?? 50).toDouble();
    final label = item.properties['label'] ?? '滑块';
    return _buildCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text('$label: ${value.toInt()}'),
          Slider(value: value, min: 0, max: 100, onChanged: null),
        ],
      ),
    );
  }

  static Widget _renderDefault(LayoutItem item) {
    return _buildCard(child: Text(item.type));
  }

  static Widget _buildCard({required Widget child}) {
    return Card(
      margin: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: child,
      ),
    );
  }
}

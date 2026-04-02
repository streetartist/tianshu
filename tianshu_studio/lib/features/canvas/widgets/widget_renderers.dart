import 'package:flutter/material.dart';
import '../../../data/models/models.dart';

/// 控件渲染器
class WidgetRenderers {
  static Widget render(WidgetInstance widget) {
    switch (widget.type) {
      case 'numberDisplay':
        return _renderNumberDisplay(widget);
      case 'textDisplay':
        return _renderTextDisplay(widget);
      case 'statusLight':
        return _renderStatusLight(widget);
      case 'progressBar':
        return _renderProgressBar(widget);
      case 'switchButton':
        return _renderSwitch(widget);
      case 'button':
        return _renderButton(widget);
      default:
        return _renderDefault(widget);
    }
  }

  static Widget _renderNumberDisplay(WidgetInstance w) {
    final value = w.properties['value'] ?? 0;
    final unit = w.properties['unit'] ?? '';
    return Center(
      child: Text(
        '$value$unit',
        style: TextStyle(fontSize: w.properties['fontSize'] ?? 24.0),
      ),
    );
  }

  static Widget _renderTextDisplay(WidgetInstance w) {
    return Center(
      child: Text(
        w.properties['text'] ?? '',
        style: TextStyle(fontSize: w.properties['fontSize'] ?? 16.0),
      ),
    );
  }

  static Widget _renderStatusLight(WidgetInstance w) {
    final status = w.properties['status'] ?? false;
    final color = status
        ? Color(int.parse((w.properties['onColor'] ?? '#4CAF50').replaceFirst('#', '0xFF')))
        : Color(int.parse((w.properties['offColor'] ?? '#9E9E9E').replaceFirst('#', '0xFF')));
    return Center(
      child: Container(
        width: 24,
        height: 24,
        decoration: BoxDecoration(shape: BoxShape.circle, color: color),
      ),
    );
  }

  static Widget _renderProgressBar(WidgetInstance w) {
    final value = (w.properties['value'] ?? 50) / 100;
    return Padding(
      padding: const EdgeInsets.all(8),
      child: LinearProgressIndicator(value: value.toDouble()),
    );
  }

  static Widget _renderSwitch(WidgetInstance w) {
    return Center(
      child: Switch(value: w.properties['value'] ?? false, onChanged: null),
    );
  }

  static Widget _renderButton(WidgetInstance w) {
    return Center(
      child: ElevatedButton(
        onPressed: null,
        child: Text(w.properties['text'] ?? '按钮'),
      ),
    );
  }

  static Widget _renderDefault(WidgetInstance w) {
    return Center(child: Text(w.type));
  }
}
